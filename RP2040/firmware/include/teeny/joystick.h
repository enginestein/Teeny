#ifndef TEENY_JOYSTICK_H
#define TEENY_JOYSTICK_H

#include <stdint.h>
#include <stdbool.h>

#include "teeny/board.h"

#ifdef __cplusplus
extern "C" {
#endif

void joystick_init(void);
uint8_t joystick_read_mask(void);
bool joystick_present(void);

#ifdef __cplusplus
}
#endif

#endif /* TEENY_JOYSTICK_H */
