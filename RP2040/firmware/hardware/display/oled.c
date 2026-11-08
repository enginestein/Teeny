#include "teeny/oled.h"

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include <string.h>

#define OLED_CMD  0x00u
#define OLED_DATA 0x40u

#define SSD1306_WIDTH   BOARD_OLED_WIDTH
#define SSD1306_HEIGHT  BOARD_OLED_HEIGHT
#define SSD1306_PAGES   (SSD1306_HEIGHT / 8u)
#define SSD1306_ROW_BYTES ((SSD1306_WIDTH + 7u) / 8u)

enum {
    CMD_DISPLAY_OFF           = 0xAE,
    CMD_DISPLAY_ON            = 0xAF,
    CMD_SET_DISPLAY_CLK_DIV   = 0xD5,
    CMD_SET_MULTIPLEX         = 0xA8,
    CMD_SET_DISPLAY_OFFSET    = 0xD3,
    CMD_SET_START_LINE        = 0x40,
    CMD_CHARGE_PUMP           = 0x8D,
    CMD_MEMORY_MODE           = 0x20,
    CMD_SEG_REMAP             = 0xA1,
    CMD_COM_SCAN_DIR_INC      = 0xC8,
    CMD_SET_COMPINS           = 0xDA,
    CMD_SET_CONTRAST          = 0x81,
    CMD_SET_PRECHARGE         = 0xD9,
    CMD_SET_VCOMH             = 0xDB,
    CMD_DISPLAY_ALL_ON_RESUME = 0xA4,
    CMD_NORMAL_DISPLAY        = 0xA6,
    CMD_SET_PAGE_ADDR         = 0xB0,
    CMD_SET_LOW_COLUMN        = 0x00,
    CMD_SET_HIGH_COLUMN       = 0x10,
};

static i2c_inst_t *s_i2c;
static uint8_t   s_addr;
static bool      s_ok;

static bool oled_cmd(uint8_t cmd)
{
    uint8_t buf[2] = { OLED_CMD, cmd };
    return i2c_write_blocking(s_i2c, s_addr, buf, sizeof(buf), false) >= 0;
}

static bool oled_data_buf(const uint8_t *data, size_t len)
{
    static uint8_t buf[1 + 128];
    if (len == 0 || len + 1 > sizeof(buf)) return false;
    buf[0] = OLED_DATA;
    memcpy(buf + 1, data, len);
    return i2c_write_blocking(s_i2c, s_addr, buf, len + 1, false) >= 0;
}

bool oled_probe(void)
{
    s_i2c = BOARD_OLED_I2C;
    s_addr = BOARD_OLED_ADDR;
    s_ok = false;

    i2c_init(s_i2c, 400000u);
    gpio_set_function(BOARD_OLED_SDA_GPIO, GPIO_FUNC_I2C);
    gpio_set_function(BOARD_OLED_SCL_GPIO, GPIO_FUNC_I2C);
    gpio_pull_up(BOARD_OLED_SDA_GPIO);
    gpio_pull_up(BOARD_OLED_SCL_GPIO);

    uint8_t buf[2] = { OLED_CMD, CMD_DISPLAY_OFF };
    int rc = i2c_write_blocking(s_i2c, s_addr, buf, sizeof(buf), false);
    s_ok = (rc >= 0);
    return s_ok;
}

bool oled_init(void)
{
    if (!s_ok) return false;

    oled_cmd(CMD_DISPLAY_OFF);
    oled_cmd(CMD_SET_DISPLAY_CLK_DIV);   oled_cmd(0x80);
    oled_cmd(CMD_SET_MULTIPLEX);         oled_cmd(SSD1306_HEIGHT - 1);
    oled_cmd(CMD_SET_DISPLAY_OFFSET);    oled_cmd(0x00);
    oled_cmd(CMD_SET_START_LINE | 0x00);
    oled_cmd(CMD_CHARGE_PUMP);           oled_cmd(0x14);
    oled_cmd(CMD_MEMORY_MODE);           oled_cmd(0x00);
    oled_cmd(CMD_SEG_REMAP | 0x01);
    oled_cmd(CMD_COM_SCAN_DIR_INC);
    oled_cmd(CMD_SET_COMPINS);           oled_cmd(0x12);
    oled_cmd(CMD_SET_CONTRAST);          oled_cmd(0xCF);
    oled_cmd(CMD_SET_PRECHARGE);         oled_cmd(0xF1);
    oled_cmd(CMD_SET_VCOMH);             oled_cmd(0x40);
    oled_cmd(CMD_DISPLAY_ALL_ON_RESUME);
    oled_cmd(CMD_NORMAL_DISPLAY);
    oled_cmd(CMD_DISPLAY_ON);

    uint8_t blank[SSD1306_WIDTH];
    memset(blank, 0, sizeof(blank));
    for (uint8_t page = 0; page < SSD1306_PAGES; page++) {
        oled_cmd(CMD_SET_PAGE_ADDR | page);
        oled_cmd(CMD_SET_LOW_COLUMN | 0x00);
        oled_cmd(CMD_SET_HIGH_COLUMN | 0x00);
        oled_data_buf(blank, sizeof(blank));
    }
    return true;
}

void oled_display_on(bool on)
{
    oled_cmd(on ? CMD_DISPLAY_ON : CMD_DISPLAY_OFF);
}

bool oled_draw_framebuffer(const uint8_t *fb1bit)
{
    if (!s_ok || fb1bit == NULL) return false;

    uint8_t page_buf[SSD1306_WIDTH];

    for (uint8_t page = 0; page < SSD1306_PAGES; page++) {
        oled_cmd(CMD_SET_PAGE_ADDR | page);
        oled_cmd(CMD_SET_LOW_COLUMN | 0x00);
        oled_cmd(CMD_SET_HIGH_COLUMN | 0x00);

        for (int col = 0; col < SSD1306_WIDTH; col++) {
            uint8_t byte = 0;
            for (int bit = 0; bit < 8; bit++) {
                int row = (int)page * 8 + bit;
                int off = row * (int)SSD1306_ROW_BYTES + col / 8;
                int col_bit = 7 - (col % 8);
                if (fb1bit[off] & (1u << col_bit)) byte |= (uint8_t)(1u << bit);
            }
            page_buf[col] = byte;
        }
        if (!oled_data_buf(page_buf, SSD1306_WIDTH)) return false;
    }
    return true;
}

bool oled_present(void)
{
    return s_ok;
}
