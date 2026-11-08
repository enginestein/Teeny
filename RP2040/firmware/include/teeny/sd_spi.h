/*
 * sd_spi.h -- SD card driver (SPI mode) for the on-board microSD slot.
 *
 * Interface is block-oriented (512-byte sectors) and uses Level-1 block
 * addressing as-is: LBA == the byte address >> 9 for high-capacity cards,
 * and the native block address for older SDSC cards.
 *
 * The FAT filesystem layer (FatFs, via the storage module's diskio glue) is
 * the only intended consumer.
 */

#ifndef TEENY_SD_SPI_H
#define TEENY_SD_SPI_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SD_OK = 0,
    SD_ERR_NOT_PRESENT, /* card never responded (no media in slot) */
    SD_ERR_TIMEOUT,     /* card stopped answering mid-operation    */
    SD_ERR_CRC,         /* data token / CRC failure                */
    SD_ERR_RANGE,       /* invalid sector range                    */
    SD_ERR_ILLEGAL,     /* card rejected a command                 */
    SD_ERR_IO           /* generic SPI/communication failure       */
} sd_error_t;

typedef enum {
    SD_CARD_STANDARD = 1, /* SDSC, block addressing via C_SIZE  */
    SD_CARD_HC          = 2, /* SDHC/SDXC, sector addressing     */
} sd_card_type_t;

typedef struct {
    bool           present;     /* media detected and initialised      */
    sd_card_type_t card_type;
    uint32_t       block_count; /* total 512-byte sectors              */
    uint64_t       capacity;    /* bytes                               */

    /* Init diagnostics (useful during bring-up / debugging). */
    sd_error_t     last_error;  /* most recent init failure (SD_OK if none) */
    uint8_t        last_cmd;    /* command that failed (0 = CMD0, etc)      */
    uint8_t        last_r1;     /* raw R1 response for last_cmd             */
    uint8_t        init_idle;   /* MISO byte sampled idle before CMD0
                                 * (0xFF = line high, 0x00 = stuck low)     */
} sd_info_t;

/* Initialise the SPI controller and bring the card to READY.
 * Re-entrant: may be called multiple times. */
sd_error_t sd_init(void);

/* Read `count` 512-byte sectors starting at LBA into dst. */
sd_error_t sd_read_sectors(uint32_t lba, uint8_t *dst, uint32_t count);

/* Write `count` 512-byte sectors starting at LBA from src. */
sd_error_t sd_write_sectors(uint32_t lba, const uint8_t *src, uint32_t count);

const sd_info_t *sd_info(void);

#ifdef __cplusplus
}
#endif

#endif /* TEENY_SD_SPI_H */