/*
 * display.h -- Game-facing display API (Phase 1: framebuffer + basic fill).
 *
 * Games draw into the framebuffer through these helpers and then call
 * display_present() to commit the frame. The presentation target is a
 * *transport* (currently USB to the PC viewer); on final hardware it will be
 * a SPI TFT. Games never touch that transport directly -- this header is the
 * only abstraction they see.
 *
 * Buffering: the module owns two whole framebuffers. Games render into the
 * "back" buffer (display_framebuffer()); presentation hands the completed
 * buffer to the transport. If the transport is still streaming the previous
 * frame the present is skipped (counted) so the game simulation never stalls
 * behind a slow or absent display link. This is the "simulation timing is
 * independent of display transport" rule from the architecture.
 *
 * Pixel format: RGB565 (16-bit 5:6:5), stored little-endian; a 160x144 frame
 * is 46,080 bytes.
 */

#ifndef TEENY_DISPLAY_H
#define TEENY_DISPLAY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "teeny/board.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DISPLAY_WIDTH  BOARD_FB_WIDTH
#define DISPLAY_HEIGHT BOARD_FB_HEIGHT

typedef uint16_t display_color_t;

/* RGB565 (5:6:5). r/g/b in 0..255. */
#define DISPLAY_COLOR(r, g, b) \
    ((display_color_t)((((uint32_t)(r) & 0xF8u) << 8) |   \
                       (((uint32_t)(g) & 0xFCu) << 3) |   \
                       (((uint32_t)(b)) >> 3)))

enum {
    DISPLAY_COLOR_BLACK   = DISPLAY_COLOR(0, 0, 0),
    DISPLAY_COLOR_WHITE   = DISPLAY_COLOR(255, 255, 255),
    DISPLAY_COLOR_RED     = DISPLAY_COLOR(255, 0, 0),
    DISPLAY_COLOR_GREEN   = DISPLAY_COLOR(0, 255, 0),
    DISPLAY_COLOR_BLUE    = DISPLAY_COLOR(0, 0, 255),
    DISPLAY_COLOR_YELLOW  = DISPLAY_COLOR(255, 255, 0),
    DISPLAY_COLOR_CYAN    = DISPLAY_COLOR(0, 255, 255),
    DISPLAY_COLOR_MAGENTA = DISPLAY_COLOR(255, 0, 255),
    DISPLAY_COLOR_GRAY    = DISPLAY_COLOR(96, 96, 96),
    DISPLAY_COLOR_DKGRAY  = DISPLAY_COLOR(48, 48, 48),
};

void display_init(void);

/* Wipe the *back* buffer (the render target) with a solid colour. */
void display_clear(display_color_t c);

/* Pixel / rectangle fill. All coordinates are clipped to the framebuffer. */
void display_set_pixel(int x, int y, display_color_t c);
void display_fill_rect(int x, int y, int w, int h, display_color_t c);

/* 5x7 bitmap text (subset: A-Z 0-9 space ! - . :). Advances 6 px per glyph.
 * If bg == fg the cells are transparent; otherwise a solid bg cell is drawn
 * behind each glyph. */
void display_draw_text(int x, int y, const char *s, display_color_t fg,
                       display_color_t bg);

/* Pointer to the frame being drawn. Do not read it while a prior frame is
 * still being presented (presented/skipped counters expose that state). */
display_color_t *display_framebuffer(void);
size_t display_framebuffer_size_bytes(void);

/* Commit the rendering back-buffer to the transport. Non-blocking: returns
 * false (and counts a skip) if the previous frame has not finished sending.
 * A skipped present means the simulation kept running but that frame was not
 * shown -- the caller should simply render the next one. */
bool display_present(void);

/* Diagnostics. */
unsigned long display_presented_frames(void);
unsigned long display_skipped_presents(void);

#ifdef __cplusplus
}
#endif

#endif /* TEENY_DISPLAY_H */