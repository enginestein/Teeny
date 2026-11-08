#ifndef TEENY_BOARD_H
#define TEENY_BOARD_H

#include "pico/stdlib.h"
#include "hardware/spi.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BOARD_STATUS_LED_GPIO
#define BOARD_STATUS_LED_GPIO (PICO_DEFAULT_LED_PIN)
#endif

#ifndef BOARD_OLED_I2C
#define BOARD_OLED_I2C i2c1
#endif
#ifndef BOARD_OLED_SDA_GPIO
#define BOARD_OLED_SDA_GPIO 4u
#endif
#ifndef BOARD_OLED_SCL_GPIO
#define BOARD_OLED_SCL_GPIO 5u
#endif
#ifndef BOARD_OLED_ADDR
#define BOARD_OLED_ADDR 0x3Cu
#endif

#define BOARD_OLED_WIDTH  128
#define BOARD_OLED_HEIGHT 64
#define BOARD_OLED_ROW_BYTES ((BOARD_OLED_WIDTH + 7u) / 8u)

#ifndef BOARD_BTN_UP_GPIO
#define BOARD_BTN_UP_GPIO 16u
#endif
#ifndef BOARD_BTN_DOWN_GPIO
#define BOARD_BTN_DOWN_GPIO 17u
#endif
#ifndef BOARD_BTN_LEFT_GPIO
#define BOARD_BTN_LEFT_GPIO 18u
#endif
#ifndef BOARD_BTN_RIGHT_GPIO
#define BOARD_BTN_RIGHT_GPIO 19u
#endif
#ifndef BOARD_BTN_A_GPIO
#define BOARD_BTN_A_GPIO 20u
#endif
#ifndef BOARD_BTN_B_GPIO
#define BOARD_BTN_B_GPIO 21u
#endif
#ifndef BOARD_BTN_START_GPIO
#define BOARD_BTN_START_GPIO 22u
#endif
#ifndef BOARD_BTN_SELECT_GPIO
#define BOARD_BTN_SELECT_GPIO 26u
#endif

#ifndef BOARD_SD_SPI_INSTANCE
#define BOARD_SD_SPI_INSTANCE spi1
#endif
#ifndef BOARD_SD_SCK_GPIO
#define BOARD_SD_SCK_GPIO PICO_SD_CLK_PIN
#endif
#ifndef BOARD_SD_MOSI_GPIO
#define BOARD_SD_MOSI_GPIO PICO_SD_CMD_PIN
#endif
#ifndef BOARD_SD_MISO_GPIO
#define BOARD_SD_MISO_GPIO PICO_SD_DAT0_PIN
#endif
#ifndef BOARD_SD_CS_GPIO
#define BOARD_SD_CS_GPIO (PICO_SD_DAT0_PIN + 3)
#endif

#define BOARD_SD_INIT_BAUD 400000u
#define BOARD_SD_BAUD 12500000u

#define BOARD_FB_WIDTH  160
#define BOARD_FB_HEIGHT 144
#define BOARD_FB_BYTES_PER_PIXEL 2
#define BOARD_FB_SIZE_BYTES ((uint32_t)(BOARD_FB_WIDTH * BOARD_FB_HEIGHT * BOARD_FB_BYTES_PER_PIXEL))

#ifdef __cplusplus
}
#endif

#endif /* TEENY_BOARD_H */
