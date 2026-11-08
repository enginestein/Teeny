/*
 * sd_spi.c -- SD card over SPI for the SparkFun Thing Plus RP2040.
 *
 * Wiring (from the Pico-SDK board definition, see teeny/board.h):
 *   SPI1: SCK=GP14, MOSI=GP15, MISO=GP12, CS=GP9
 *
 * The card is brought up in SPI mode: 80 dummy clocks, CMD0 to enter SPI
 * mode, CMD8/ACMD41 to detect SDv2.0+ high-capacity cards, then the clock
 * is raised from 400 kHz (required for init) to the running speed.
 *
 * Command CRC bytes: only CMD0 and CMD8 require a valid 7-bit CRC; every
 * other command's CRC byte is ignored in SPI mode. Data tokens use the
 * standard 0xFE (single block), 0xFC (multi block) markers.
 *
 * Timeouts use busy-waits on the microsecond counter. Read/write timeouts
 * are deliberately generous: SD cards may take up to ~250 ms for a block
 * erase/program cycle.
 *
 * Capacity: CSCD/CSD parsing. Only used for diagnostics; a wrong value here
 * must never affect read/write correctness (FatFs drives by sector numbers).
 */

#include "teeny/sd_spi.h"
#include "teeny/board.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include <string.h>

#define SD_SECTOR_SIZE 512u

#define SD_TIMEOUT_US 500000u      /* general command timeout (0.5 s)   */
#define SD_DATA_TIMEOUT_US 1500000u/* data token timeout                */

#define R1_IDLE              (1u << 0)
#define R1_ILLEGAL_CMD       (1u << 2)
#define R1_BIT7              (1u << 7)

#define START_BLOCK_SINGLE   0xFEu
#define START_BLOCK_MULTI    0xFCu
#define STOP_TRAN            0xFDu
#define DAT_ACCEPTED_MASK    0x1Fu
#define DAT_ACCEPTED         0x05
#define OCR_CCS              (1u << 30)

static sd_info_t s_info;

#define SCK  (BOARD_SD_SCK_GPIO)
#define MOSI (BOARD_SD_MOSI_GPIO)
#define MISO (BOARD_SD_MISO_GPIO)
#define CS   (BOARD_SD_CS_GPIO)

static inline void cs_low(void)  { gpio_put(CS, 0); }
static inline void cs_high(void) { gpio_put(CS, 1); }

/* ------------------------------------------------------------------------- *
 * Bit-banged SPI transport.
 *
 * The SparkFun Thing Plus SD slot is notorious for marginal SD init when
 * driven through the SPI peripheral, while direct GPIO bit-banging of the
 * same pins works (this matches the MicroPython experience on this board).
 * Bit-banging sidesteps any peripheral/mux ambiguity and is slow but fully
 * deterministic. Command + data phases both run through here; 512-byte
 * sectors transfer in ~4 ms at ~1 MHz.
 * ------------------------------------------------------------------------- */

static void bb_delay(void)
{
    for (volatile int i = 0; i < 6; i++) {
    }
}

static uint8_t bb_xfer(uint8_t out)
{
    uint8_t in = 0;
    for (int bit = 7; bit >= 0; bit--) {
        gpio_put(MOSI, (out >> bit) & 1u);
        gpio_put(SCK, 1);
        bb_delay();
        in = (uint8_t)((in << 1) | (gpio_get(MISO) ? 1u : 0u));
        gpio_put(SCK, 0);
        bb_delay();
    }
    return in;
}

static uint8_t xfer_byte(uint8_t out)
{
    return bb_xfer(out);
}

static void clock_dummy(int n)
{
    while (n-- > 0) {
        xfer_byte(0xFF);
    }
}

/* Transmit a command and read its R1 response.
 * CS must already be asserted. */
static sd_error_t sd_cmd(uint8_t cmd, uint32_t arg, uint8_t *r1)
{
    uint8_t buf[7];

    buf[0] = 0x40U | (cmd & 0x3F);
    buf[1] = (uint8_t)(arg >> 24);
    buf[2] = (uint8_t)(arg >> 16);
    buf[3] = (uint8_t)(arg >> 8);
    buf[4] = (uint8_t)arg;

    /* CRC byte: required for CMD0 and CMD8 (see file header). */
    if (cmd == 0) {
        buf[5] = 0x95u;
    } else if (cmd == 8) {
        buf[5] = 0x87u;
    } else {
        buf[5] = 0xFFu;
    }
    buf[6] = 0xFFu; /* trailing clock */

    for (int i = 0; i < 7; i++) {
        xfer_byte(buf[i]);
    }

    /* R1: read 0 bytes whose MSB is set, then the response. */
    uint64_t deadline = time_us_64() + SD_TIMEOUT_US;
    uint8_t b;
    do {
        b = xfer_byte(0xFF);
        if (time_us_64() > deadline) {
            *r1 = 0xFF;
            return SD_ERR_TIMEOUT;
        }
    } while (b & R1_BIT7);

    *r1 = b & 0x7Fu;
    return (b & R1_BIT7) ? SD_ERR_TIMEOUT : SD_OK;
}

/* SD SpeC: capacity from the CSD register returned by CMD9. */
static sd_error_t parse_csd_block_count(uint8_t *csd, uint32_t *blocks)
{
    const uint8_t csd_structure = csd[0] >> 6;

    if (csd_structure == 0) {
        /* SDSC v1.0 CSD: C_SIZE = bits [73:62], C_SIZE_MULT=[49:47],
         * READ_BL_LEN=[83:80]. Formula in use here matches the widely
         * deployed sd_card driver for the RP2040. */
        uint32_t read_bl_len = (csd[5] & 0x0Fu) + 2u; /* *4 bytes */
        uint32_t c_size = ((uint32_t)(csd[6] & 0x03u) << 10u) |
                          ((uint32_t)csd[7] << 2u) |
                          ((uint32_t)(csd[8] & 0xC0u) >> 6u);
        uint32_t c_size_mult = ((uint32_t)(csd[9] & 0x03u) << 1u) |
                               ((uint32_t)(csd[10] & 0x80u) >> 7u);
        uint32_t block_nr = (c_size + 1u) << (c_size_mult + 2u);
        uint32_t block_len = 1u << read_bl_len;
        *blocks = block_nr * (block_len / SD_SECTOR_SIZE);
        return SD_OK;
    }
    if (csd_structure == 1) {
        /* CSD v2.0 for SDHC/SDXC: 22-bit C_SIZE at top bits of
         * csd[9], csd[8], csd[7]. */
        uint32_t c_size = ((uint32_t)(csd[7] & 0x3Fu) << 16u) |
                          ((uint32_t)csd[8] << 8u) |
                          (uint32_t)csd[9];
        *blocks = (c_size + 1u) * 1024u; /* 512-byte sectors */
        return SD_OK;
    }
    return SD_ERR_ILLEGAL;
}

static sd_error_t read_csd(uint32_t *blocks)
{
    uint8_t csd[16];
    uint8_t r1;

    cs_low();
    sd_error_t e = sd_cmd(9, 0, &r1);
    if (e != SD_OK || r1 != 0x00) {
        cs_high();
        return (e != SD_OK) ? e : SD_ERR_ILLEGAL;
    }

    /* Token then 16 data bytes + 2 CRC bytes. */
    uint64_t deadline = time_us_64() + SD_DATA_TIMEOUT_US;
    uint8_t b;
    do {
        b = xfer_byte(0xFF);
        if (time_us_64() > deadline) {
            cs_high();
            return SD_ERR_TIMEOUT;
        }
    } while (b != 0xFE);

    for (int i = 0; i < 16; i++) {
        csd[i] = xfer_byte(0xFF);
    }
    xfer_byte(0xFF); /* CRC low  */
    xfer_byte(0xFF); /* CRC high */
    cs_high();

    e = parse_csd_block_count(csd, blocks);
    return e;
}

static void record_fail(uint8_t cmd, uint8_t r1, sd_error_t err)
{
    s_info.last_cmd = cmd;
    s_info.last_r1 = r1;
    s_info.last_error = err;
}

static sd_error_t card_init(void)
{
    sd_error_t e;
    uint8_t r1;
    uint32_t ocr = 0;

    /* Reset / re-init sequence. CS released, at least 80 clocks. */
    cs_high();
    clock_dummy(16);
    sleep_ms(2);

    /* What does MISO look like before we even begin? 0xFF = line held high
     * (card present but idle). 0x00 = stuck low (wiring/slot problem). */
    s_info.init_idle = xfer_byte(0xFF);

    /* CMD0: enter SPI mode. */
    cs_low();
    e = sd_cmd(0, 0, &r1);
    if (e == SD_ERR_TIMEOUT || r1 == 0xFF) {
        cs_high();
        record_fail(0, r1, SD_ERR_NOT_PRESENT); /* nothing echoes -> no card */
        return SD_ERR_NOT_PRESENT;
    }
    if (r1 != R1_IDLE) {
        cs_high();
        record_fail(0, r1, SD_ERR_IO);
        return SD_ERR_IO;
    }

    /* CMD8: check SD v2.0+ support. */
    e = sd_cmd(8, 0x000001AAu, &r1);
    uint8_t r7[4];
    if (e == SD_OK && r1 == R1_IDLE) {
        /* Read the 32-bit R7 echo (must be 0x00 0x00 0x01 0xAA). */
        r7[0] = xfer_byte(0xFF);
        r7[1] = xfer_byte(0xFF);
        r7[2] = xfer_byte(0xFF);
        r7[3] = xfer_byte(0xFF);
        if (r7[0] != 0x00 || r7[1] != 0x00 || r7[2] != 0x01 || r7[3] != 0xAA) {
            cs_high();
            record_fail(8, r1, SD_ERR_IO);
            return SD_ERR_IO;
        }
        s_info.card_type = SD_CARD_HC;
    } else {
        /* SD v1.x: illegal command (R1 bit2) or no echo. */
        if (r1 & R1_ILLEGAL_CMD) {
            s_info.card_type = SD_CARD_STANDARD;
        } else {
            cs_high();
            record_fail(8, r1, SD_ERR_IO);
            return SD_ERR_IO;
        }
    }

    /* ACMD41 with HCS until card ready. */
    uint64_t deadline = time_us_64() + SD_TIMEOUT_US;
    do {
        e = sd_cmd(55, 0, &r1);          /* CMD55: next is app command */
        if (e != SD_OK) {
            cs_high();
            record_fail(55, r1, e);
            return e;
        }
        e = sd_cmd(41, 0x40000000u, &r1); /* ACMD41 + HCS */
        if (e != SD_OK) {
            cs_high();
            record_fail(41, r1, e);
            return e;
        }
    } while ((r1 & R1_IDLE) && time_us_64() < deadline);

    if (r1 & R1_IDLE) {
        cs_high();
        record_fail(41, r1, SD_ERR_TIMEOUT);
        return SD_ERR_TIMEOUT;
    }

    /* CMD58: read OCR to confirm the command set. HC -> block addressing. */
    if (s_info.card_type == SD_CARD_HC) {
        e = sd_cmd(58, 0, &r1);
        if (e != SD_OK || r1 != 0x00) {
            cs_high();
            record_fail(58, r1, (e != SD_OK) ? e : SD_ERR_IO);
            return (e != SD_OK) ? e : SD_ERR_IO;
        }
        ocr = ((uint32_t)xfer_byte(0xFF) << 24) |
              ((uint32_t)xfer_byte(0xFF) << 16) |
              ((uint32_t)xfer_byte(0xFF) << 8) |
              (uint32_t)xfer_byte(0xFF);
        if (!(ocr & OCR_CCS)) {
            s_info.card_type = SD_CARD_STANDARD;
        }
    }
    cs_high();

    /* Nothing to change here for bit-bang: the transport stays deliberately
     * slow and deterministic. Init and data share the same bit-bang clock. */

    uint32_t blocks = 0;
    e = read_csd(&blocks);
    if (e != SD_OK || blocks == 0) {
        record_fail(9, 0x00, (e != SD_OK) ? e : SD_ERR_IO);
        return e != SD_OK ? e : SD_ERR_IO;
    }
    s_info.block_count = blocks;
    s_info.capacity = (uint64_t)blocks * SD_SECTOR_SIZE;
    s_info.present = true;
    s_info.last_error = SD_OK;
    s_info.last_cmd = 0;
    s_info.last_r1 = 0x00;
    return SD_OK;
}

sd_error_t sd_init(void)
{
    s_info.present = false;
    s_info.block_count = 0;
    s_info.capacity = 0;
    s_info.card_type = SD_CARD_STANDARD;
    s_info.last_error = SD_OK;
    s_info.last_cmd = 0;
    s_info.last_r1 = 0x00;
    s_info.init_idle = 0xFF;

    gpio_init(SCK);
    gpio_init(MOSI);
    gpio_init(MISO);
    gpio_init(CS);
    gpio_set_dir(SCK, GPIO_OUT);
    gpio_set_dir(MOSI, GPIO_OUT);
    gpio_set_dir(MISO, GPIO_IN);
    gpio_set_dir(CS, GPIO_OUT);
    gpio_pull_up(MISO);
    cs_high();

    return card_init();
}

/* Translate a block number to the address word sent in CMD17/18/24:
 * SDHC/SDXC use block addressing directly, SDSC use byte addressing. */
static inline uint32_t sd_addr(uint32_t lba)
{
    return (s_info.card_type == SD_CARD_STANDARD) ? (lba << 9) : lba;
}

/* Wait for a data token from the block-read/write state. Returns the first
 * non-0xFF byte seen (the data token). */
static sd_error_t wait_token(uint8_t *tok)
{
    uint64_t deadline = time_us_64() + SD_DATA_TIMEOUT_US;
    *tok = 0xFF;
    do {
        *tok = xfer_byte(0xFF);
        if (time_us_64() > deadline) {
            return SD_ERR_TIMEOUT;
        }
    } while (*tok == 0xFF);
    return SD_OK;
}

sd_error_t sd_read_sectors(uint32_t lba, uint8_t *dst, uint32_t count)
{
    if (!s_info.present || count == 0) {
        return SD_ERR_IO;
    }
    if ((uint64_t)lba + (uint64_t)count > s_info.block_count) {
        return SD_ERR_RANGE;
    }

    uint8_t r1;
    sd_error_t e;

    cs_low();
    e = sd_cmd(18, sd_addr(lba), &r1);          /* CMD18: read multiple blocks */
    if (e != SD_OK || r1 != 0x00) {
        cs_high();
        return (e != SD_OK) ? e : SD_ERR_ILLEGAL;
    }

    for (uint32_t i = 0; i < count; i++) {
        uint8_t tok;
        e = wait_token(&tok);
        if (e != SD_OK || tok != START_BLOCK_SINGLE) {
            cs_high();
            return (e != SD_OK) ? e : SD_ERR_CRC;
        }
        for (uint32_t j = 0; j < SD_SECTOR_SIZE; j++) {
            dst[j] = xfer_byte(0xFF);
        }
        dst += SD_SECTOR_SIZE;
        xfer_byte(0xFF); /* CRC */
        xfer_byte(0xFF);
    }

    /* Stop transmission. */
    sd_cmd(12, 0, &r1);
    cs_high();
    clock_dummy(2);
    return SD_OK;
}

/* Returns true while the card reports busy (data line low). */
static bool card_busy(void)
{
    return xfer_byte(0xFF) == 0x00;
}

static sd_error_t wait_not_busy(void)
{
    uint64_t deadline = time_us_64() + SD_DATA_TIMEOUT_US;
    while (card_busy()) {
        if (time_us_64() > deadline) {
            return SD_ERR_TIMEOUT;
        }
    }
    return SD_OK;
}

sd_error_t sd_write_sectors(uint32_t lba, const uint8_t *src, uint32_t count)
{
    if (!s_info.present || count == 0) {
        return SD_ERR_IO;
    }
    if ((uint64_t)lba + (uint64_t)count > s_info.block_count) {
        return SD_ERR_RANGE;
    }

    uint8_t r1;
    sd_error_t e;
    uint8_t token[SD_SECTOR_SIZE + 2];
    const uint8_t start_token = START_BLOCK_SINGLE;
    const uint8_t stop_token  = STOP_TRAN;

    cs_low();

    for (uint32_t i = 0; i < count; i++) {
        if (sd_cmd(24, sd_addr(lba + i), &r1) != SD_OK || r1 != 0x00) {
            cs_high();
            return SD_ERR_ILLEGAL;
        }
        memcpy(token, src, SD_SECTOR_SIZE);
        token[SD_SECTOR_SIZE] = 0xFF;   /* CRC (ignored in SPI) */
        token[SD_SECTOR_SIZE + 1] = 0xFF;
        xfer_byte(start_token);
        for (int k = 0; k < (int)sizeof(token); k++) {
            xfer_byte(token[k]);
        }

        r1 = xfer_byte(0xFF);
        if ((r1 & DAT_ACCEPTED_MASK) != DAT_ACCEPTED) {
            cs_high();
            return SD_ERR_CRC;
        }
        e = wait_not_busy();
        if (e != SD_OK) {
            cs_high();
            return e;
        }
        src += SD_SECTOR_SIZE;
    }

    /* Terminate (only meaningful for multi-block writes). */
    xfer_byte(stop_token);
    cs_high();
    clock_dummy(2);
    return SD_OK;
}

const sd_info_t *sd_info(void)
{
    return &s_info;
}