/*
 * phase1_pattern.c -- Phase-1 test pattern renderer.
 *
 * 160x144 layout:
 *   y 0..27    6 colour bars (red/green/blue/yellow/cyan/magenta)
 *   y 28..43   horizontal grey ramp (black -> white)
 *   y 44..99   checkerboard (2 greys)
 *   y 100..143 motion band: bouncing ball + sweeping scanline
 */

#include "teeny/phase1_pattern.h"

#include "teeny/display.h"

void phase1_pattern_render(uint32_t now_ms)
{
    display_clear(DISPLAY_COLOR_BLACK);

    /* Colour bars. */
    const display_color_t bars[6] = {
        DISPLAY_COLOR_RED, DISPLAY_COLOR_GREEN, DISPLAY_COLOR_BLUE,
        DISPLAY_COLOR_YELLOW, DISPLAY_COLOR_CYAN, DISPLAY_COLOR_MAGENTA,
    };
    const int bar_count = 6;
    const int bar_w = (DISPLAY_WIDTH + bar_count - 1) / bar_count;
    for (int i = 0; i < bar_count; i++) {
        display_fill_rect(i * bar_w, 0, bar_w, 28, bars[i]);
    }

    /* Grey ramp. */
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
        const uint8_t luma = (uint8_t)((x * 255u) / (DISPLAY_WIDTH - 1u));
        const display_color_t c = DISPLAY_COLOR(luma, luma, luma);
        for (int y = 28; y < 44; y++) {
            display_set_pixel(x, y, c);
        }
    }

    /* Checkerboard. */
    for (int gy = 0; gy < 6; gy++) {
        for (int gx = 0; gx < 8; gx++) {
            const display_color_t c =
                ((gx + gy) & 1) ? DISPLAY_COLOR_GRAY : DISPLAY_COLOR_DKGRAY;
            display_fill_rect(gx * 20, 44 + gy * 10, 20, 10, c);
        }
    }

    /* Motion band (y 100..143). */
    const int band_y = 100;
    const int band_h = DISPLAY_HEIGHT - band_y;

    /* Sweeping scanline: a bright 2px line moving down the band. */
    const int scan_y = band_y + (int)((now_ms / 16u) % (uint32_t)band_h);
    display_fill_rect(0, scan_y, DISPLAY_WIDTH, 2, DISPLAY_COLOR_CYAN);

    /* Bouncing ball: a 9x9 px box moving on an integer Lissajous path. */
    const uint32_t t = now_ms / 32u;
    const int bx = (int)((t * 37u) % (uint32_t)(DISPLAY_WIDTH - 10));
    const int by = band_y + (int)((t * 29u) % (uint32_t)(band_h - 10));
    display_fill_rect(bx, by, 10, 10, DISPLAY_COLOR_WHITE);
    display_fill_rect(bx + 2, by + 2, 6, 6, DISPLAY_COLOR_RED);
}