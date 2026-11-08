#include "teeny/joystick.h"

#include "pico/stdlib.h"

typedef struct {
    uint gpio;
    uint8_t bit;
} btn_map_t;

static const btn_map_t s_btn_map[] = {
    { BOARD_BTN_RIGHT_GPIO, 0u },
    { BOARD_BTN_LEFT_GPIO,  1u },
    { BOARD_BTN_UP_GPIO,    2u },
    { BOARD_BTN_DOWN_GPIO,  3u },
    { BOARD_BTN_A_GPIO,     4u },
    { BOARD_BTN_B_GPIO,     5u },
    { BOARD_BTN_START_GPIO, 6u },
    { BOARD_BTN_SELECT_GPIO, 7u },
};
#define NUM_BTNS ((int)(sizeof(s_btn_map) / sizeof(s_btn_map[0])))

static bool s_present;

void joystick_init(void)
{
    s_present = true;
    for (int i = 0; i < NUM_BTNS; i++) {
        gpio_init(s_btn_map[i].gpio);
        gpio_set_dir(s_btn_map[i].gpio, GPIO_IN);
        gpio_pull_up(s_btn_map[i].gpio);
    }
}

uint8_t joystick_read_mask(void)
{
    uint8_t mask = 0;
    for (int i = 0; i < NUM_BTNS; i++) {
        if (!gpio_get(s_btn_map[i].gpio)) mask |= (uint8_t)(1u << s_btn_map[i].bit);
    }
    return mask;
}

bool joystick_present(void)
{
    return s_present;
}
