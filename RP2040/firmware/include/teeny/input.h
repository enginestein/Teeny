/*
 * input.h -- Abstract retro-console buttons for Teeny.
 *
 * Games never see a keyboard or a wire protocol; they poll a small set of
 * logical console buttons (D-pad + A/B/START/SELECT). On the dev rig the PC
 * keyboard is mapped to these buttons in the host viewer and shipped over the
 * USB link as PKT_INPUT; on a real console a GPIO matrix could feed the same
 * buttons. Input state is a plain byte (one bit per button) latched by the
 * link layer; the game loop samples it each fixed tick.
 */

#ifndef TEENY_INPUT_H
#define TEENY_INPUT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Logical buttons. Bit positions are the protocol encoding, keep them stable. */
typedef enum {
    BTN_RIGHT = 0, /* bit 0 */
    BTN_LEFT  = 1, /* bit 1 */
    BTN_UP    = 2, /* bit 2 */
    BTN_DOWN  = 3, /* bit 3 */
    BTN_A     = 4, /* bit 4 */
    BTN_B     = 5, /* bit 5 */
    BTN_START = 6, /* bit 6 */
    BTN_SELECT = 7 /* bit 7 */
} console_button_t;

#define INPUT_MASK_BIT(btn) (uint8_t)(1u << (btn))

void  input_init(void);
void  input_set_mask(uint8_t mask);
uint8_t input_mask(void);
bool  input_is_pressed(console_button_t btn);

/* Fixed-timestep helpers. Call input_tick() once per simulation tick (the
 * kernel does this); it latches the current mask as the "previous" state.
 * input_edge_pressed() then reports a button that became pressed this tick --
 * the classic single-trigger used for launch/confirm/shoot, so a held key
 * does not auto-fire repeat. */
void  input_tick(void);
bool  input_edge_pressed(console_button_t btn);

#ifdef __cplusplus
}
#endif

#endif /* TEENY_INPUT_H */