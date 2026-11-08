#ifndef TEENY_OLED_H
#define TEENY_OLED_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "teeny/board.h"

#ifdef __cplusplus
extern "C" {
#endif

bool oled_probe(void);
bool oled_init(void);
bool oled_draw_framebuffer(const uint8_t *fb1bit);
void oled_display_on(bool on);
bool oled_present(void);

#ifdef __cplusplus
}
#endif

#endif /* TEENY_OLED_H */
