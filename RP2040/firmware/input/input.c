#include "teeny/input.h"

#include "teeny/mode.h"
#include "teeny/joystick.h"

static uint8_t s_mask;
static uint8_t s_latch;
static uint8_t s_prev;

void input_init(void)
{
    s_mask = 0;
    s_latch = 0;
    s_prev = 0;
    if (g_system_mode == MODE_HARDWARE) joystick_init();
}

void input_set_mask(uint8_t mask)
{
    s_mask = mask;
}

uint8_t input_mask(void)
{
    return s_mask;
}

bool input_is_pressed(console_button_t btn)
{
    return (s_mask & INPUT_MASK_BIT(btn)) != 0;
}

void input_tick(void)
{
    if (g_system_mode == MODE_HARDWARE) {
        s_mask = joystick_read_mask();
    }
    s_prev = s_latch;
    s_latch = s_mask;
}

bool input_edge_pressed(console_button_t btn)
{
    const uint8_t bit = INPUT_MASK_BIT(btn);
    return (s_latch & bit) != 0 && (s_prev & bit) == 0;
}
