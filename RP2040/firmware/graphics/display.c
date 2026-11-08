#include "teeny/display.h"

#include "teeny/video_out.h"
#include "teeny/mode.h"
#include "teeny/oled.h"

#include <string.h>

#define FB_PIXELS ((size_t)DISPLAY_WIDTH * (size_t)DISPLAY_HEIGHT)

static display_color_t s_buf_a[FB_PIXELS];
static display_color_t s_buf_b[FB_PIXELS];

static display_color_t *s_draw;
static unsigned long s_presented;
static unsigned long s_skipped;

static uint8_t s_oled_fb[(BOARD_OLED_WIDTH * BOARD_OLED_HEIGHT) / 8u];

void display_init(void)
{
    s_draw = s_buf_a;
    s_presented = 0;
    s_skipped = 0;
    for (size_t i = 0; i < FB_PIXELS; i++) {
        s_buf_a[i] = DISPLAY_COLOR_BLACK;
        s_buf_b[i] = DISPLAY_COLOR_BLACK;
    }
    if (g_system_mode == MODE_HARDWARE) {
        oled_init();
        oled_display_on(true);
    }
}

display_color_t *display_framebuffer(void)
{
    return s_draw;
}

size_t display_framebuffer_size_bytes(void)
{
    return FB_PIXELS * sizeof(display_color_t);
}

void display_clear(display_color_t c)
{
    for (size_t i = 0; i < FB_PIXELS; i++) s_draw[i] = c;
}

void display_set_pixel(int x, int y, display_color_t c)
{
    if (x < 0 || x >= DISPLAY_WIDTH || y < 0 || y >= DISPLAY_HEIGHT) return;
    s_draw[(size_t)y * DISPLAY_WIDTH + (size_t)x] = c;
}

void display_fill_rect(int x, int y, int w, int h, display_color_t c)
{
    if (w <= 0 || h <= 0) return;
    int x1 = x + w, y1 = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 > DISPLAY_WIDTH) x1 = DISPLAY_WIDTH;
    if (y1 > DISPLAY_HEIGHT) y1 = DISPLAY_HEIGHT;
    if (x >= x1 || y >= y1) return;
    for (int row = y; row < y1; row++) {
        display_color_t *line = &s_draw[(size_t)row * DISPLAY_WIDTH];
        for (int col = x; col < x1; col++) line[col] = c;
    }
}

typedef struct {
    char ch;
    uint8_t g[5];
} font_glyph_t;

#define FONT_GLYPH(c, a0, a1, a2, a3, a4) { (c), { (a0), (a1), (a2), (a3), (a4) } }

static const font_glyph_t s_font[] = {
    FONT_GLYPH(' ', 0x00, 0x00, 0x00, 0x00, 0x00),
    FONT_GLYPH('!', 0x00, 0x00, 0x5F, 0x00, 0x00),
    FONT_GLYPH('-', 0x08, 0x08, 0x08, 0x08, 0x08),
    FONT_GLYPH('.', 0x00, 0x00, 0x60, 0x60, 0x00),
    FONT_GLYPH(':', 0x00, 0x00, 0x36, 0x36, 0x00),
    FONT_GLYPH('0', 0x3E, 0x45, 0x49, 0x51, 0x3E),
    FONT_GLYPH('1', 0x00, 0x40, 0x7F, 0x42, 0x00),
    FONT_GLYPH('2', 0x46, 0x49, 0x49, 0x51, 0x62),
    FONT_GLYPH('3', 0x36, 0x49, 0x49, 0x49, 0x41),
    FONT_GLYPH('4', 0x10, 0x7F, 0x12, 0x14, 0x18),
    FONT_GLYPH('5', 0x39, 0x45, 0x45, 0x45, 0x27),
    FONT_GLYPH('6', 0x30, 0x49, 0x49, 0x4A, 0x3C),
    FONT_GLYPH('7', 0x03, 0x05, 0x09, 0x71, 0x01),
    FONT_GLYPH('8', 0x36, 0x49, 0x49, 0x49, 0x36),
    FONT_GLYPH('9', 0x1E, 0x29, 0x49, 0x49, 0x06),
    FONT_GLYPH('A', 0x7E, 0x09, 0x09, 0x09, 0x7E),
    FONT_GLYPH('B', 0x36, 0x49, 0x49, 0x49, 0x7F),
    FONT_GLYPH('C', 0x22, 0x41, 0x41, 0x41, 0x3E),
    FONT_GLYPH('D', 0x1C, 0x22, 0x41, 0x41, 0x7F),
    FONT_GLYPH('E', 0x41, 0x49, 0x49, 0x49, 0x7F),
    FONT_GLYPH('F', 0x01, 0x09, 0x09, 0x09, 0x7F),
    FONT_GLYPH('G', 0x39, 0x49, 0x49, 0x41, 0x3E),
    FONT_GLYPH('H', 0x7F, 0x08, 0x08, 0x08, 0x7F),
    FONT_GLYPH('I', 0x00, 0x41, 0x7F, 0x41, 0x00),
    FONT_GLYPH('J', 0x01, 0x3F, 0x41, 0x40, 0x20),
    FONT_GLYPH('K', 0x41, 0x22, 0x14, 0x08, 0x7F),
    FONT_GLYPH('L', 0x40, 0x40, 0x40, 0x40, 0x7F),
    FONT_GLYPH('M', 0x7F, 0x02, 0x0C, 0x02, 0x7F),
    FONT_GLYPH('N', 0x7F, 0x10, 0x08, 0x04, 0x7F),
    FONT_GLYPH('O', 0x3E, 0x41, 0x41, 0x41, 0x3E),
    FONT_GLYPH('P', 0x06, 0x09, 0x09, 0x09, 0x7F),
    FONT_GLYPH('Q', 0x5E, 0x21, 0x51, 0x41, 0x3E),
    FONT_GLYPH('R', 0x46, 0x29, 0x19, 0x09, 0x7F),
    FONT_GLYPH('S', 0x31, 0x49, 0x49, 0x49, 0x46),
    FONT_GLYPH('T', 0x01, 0x01, 0x7F, 0x01, 0x01),
    FONT_GLYPH('U', 0x3F, 0x40, 0x40, 0x40, 0x3F),
    FONT_GLYPH('V', 0x1F, 0x20, 0x40, 0x20, 0x1F),
    FONT_GLYPH('W', 0x7F, 0x20, 0x18, 0x20, 0x7F),
    FONT_GLYPH('X', 0x63, 0x14, 0x08, 0x14, 0x63),
    FONT_GLYPH('Y', 0x03, 0x04, 0x78, 0x04, 0x03),
    FONT_GLYPH('Z', 0x43, 0x45, 0x49, 0x51, 0x61),
};

static const font_glyph_t *font_find(char c)
{
    for (size_t i = 0; i < sizeof(s_font) / sizeof(s_font[0]); i++) {
        if (s_font[i].ch == c) return &s_font[i];
    }
    return &s_font[0];
}

void display_draw_text(int x, int y, const char *s, display_color_t fg,
                       display_color_t bg)
{
    while (*s != '\0') {
        const font_glyph_t *gl = font_find(*s);
        if (bg != fg) display_fill_rect(x, y, 6, 8, bg);
        for (int col = 0; col < 5; col++) {
            uint8_t bits = gl->g[col];
            for (int row = 0; row < 7; row++) {
                if ((bits & (1u << row)) != 0) {
                    display_set_pixel(x + col, y + 1 + row, fg);
                }
            }
        }
        x += 6;
        s++;
    }
}

bool display_present(void)
{
    if (g_system_mode == MODE_HARDWARE) {
        memset(s_oled_fb, 0, sizeof(s_oled_fb));
        for (int oy = 0; oy < BOARD_OLED_HEIGHT; oy++) {
            for (int ox = 0; ox < BOARD_OLED_WIDTH; ox++) {
                int sx = (ox * DISPLAY_WIDTH) / BOARD_OLED_WIDTH;
                int sy = (oy * DISPLAY_HEIGHT) / BOARD_OLED_HEIGHT;
                display_color_t px = s_draw[(size_t)sy * DISPLAY_WIDTH + (size_t)sx];
                uint8_t r = (uint8_t)((px >> 11) & 0x1Fu) << 3;
                uint8_t g = (uint8_t)((px >> 5)  & 0x3Fu) << 2;
                uint8_t b = (uint8_t)( px        & 0x1Fu) << 3;
                uint8_t lum = (uint8_t)((30u * r + 59u * g + 11u * b) / 100u);
                if (lum > 128u) {
                    int byte_idx = oy * (int)BOARD_OLED_ROW_BYTES + ox / 8;
                    s_oled_fb[byte_idx] |= (uint8_t)(1u << (7 - (ox % 8)));
                }
            }
        }
        if (!oled_draw_framebuffer(s_oled_fb)) {
            s_skipped++;
            return false;
        }
        s_draw = (s_draw == s_buf_a) ? s_buf_b : s_buf_a;
        s_presented++;
        return true;
    }

    if (video_out_busy()) {
        s_skipped++;
        return false;
    }
    s_draw = (s_draw == s_buf_a) ? s_buf_b : s_buf_a;
    if (!video_out_start((const uint8_t *)s_draw)) {
        s_skipped++;
        return false;
    }
    s_presented++;
    return true;
}

unsigned long display_presented_frames(void)
{
    return s_presented;
}

unsigned long display_skipped_presents(void)
{
    return s_skipped;
}
