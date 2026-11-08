/*
 * tetris.c -- Tetris for Teeny.
 *
 * A 10x18 playfield of 7px cells centered in the 160x144 frame with a 16px
 * HUD strip on top (score + next-piece preview). Pieces are 4x4 bitmasks;
 * the four rotations per type are derived once at init with a 90-degree
 * rotation of the mask. Controls: LEFT/RIGHT move (held keys auto-repeat),
 * A rotates, B hard-drops, DOWN soft-drops. Lines clear Classic-style with
 * 100/300/500/800 scoring; gravity eases as you clear lines.
 */

#include "teeny/tetris.h"

#include "teeny/display.h"
#include "teeny/input.h"

#include "pico/stdlib.h"

#include <stdint.h>
#include <stdio.h>

#define HUD_PX      16
#define COLS        10
#define ROWS        18
#define CELL        7
#define PF_X        45
#define PF_Y        (HUD_PX + 1)
#define PIECE_COUNT 7
#define GAP_PX      (PF_X + COLS * CELL)

/* Base (rotation 0) orientation of each tetromino in a 4x4 box. Bit n of a
 * row word is cell column n. Bits are stored (row0<<12 | row1<<8 | row2<<4 |
 * row3); row0 is the top of the box. */
static const uint16_t s_base[PIECE_COUNT] = {
    0x0F00u, /* I */
    0x0660u, /* O */
    0x0E40u, /* T */
    0x0630u, /* S */
    0x0360u, /* Z */
    0x0170u, /* J */
    0x08E0u, /* L */
};

static const display_color_t s_colors[PIECE_COUNT] = {
    DISPLAY_COLOR_CYAN, DISPLAY_COLOR_YELLOW, DISPLAY_COLOR_MAGENTA,
    DISPLAY_COLOR_GREEN, DISPLAY_COLOR_RED, DISPLAY_COLOR_BLUE,
    DISPLAY_COLOR_WHITE,
};

static uint16_t s_pieces[PIECE_COUNT][4];
static uint8_t  s_grid[COLS][ROWS]; /* 0 = empty, else piece type + 1 */
static int      s_type;
static int      s_rot;
static int      s_px;  /* top-left cell column of the falling piece */
static int      s_py;  /* top-left cell row (may be -1 while entering) */
static int      s_next;
static uint16_t s_acc;
static uint16_t s_drop_ticks;
static uint16_t s_lines;
static unsigned s_score;
static bool     s_over;
static uint8_t  s_hold_ticks;
static uint32_t s_rand;

static uint32_t prng_next(void)
{
    uint32_t x = s_rand;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s_rand = x != 0 ? x : 0xA5A5A5A5u;
    return s_rand;
}

static uint16_t rot_cw(uint16_t m)
{
    uint16_t out = 0;
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (m & (1u << (r * 4 + c))) {
                out |= (uint16_t)(1u << (c * 4 + (3 - r)));
            }
        }
    }
    return out;
}

static bool collides(int type, int rot, int x, int y)
{
    const uint16_t m = s_pieces[type][rot];
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            if (!(m & (1u << (r * 4 + c)))) {
                continue;
            }
            const int gx = x + c;
            const int gy = y + r;
            if (gx < 0 || gx >= COLS || gy >= ROWS) {
                return true;
            }
            if (gy >= 0 && s_grid[gx][gy] != 0) {
                return true;
            }
        }
    }
    return false;
}

static void spawn(void)
{
    s_type = s_next;
    s_next = (int)(prng_next() % PIECE_COUNT);
    s_rot = 0;
    s_px = 3;
    s_py = -1;
    if (collides(s_type, s_rot, s_px, s_py)) {
        s_over = true;
    }
}

void tetris_init(void)
{
    for (int t = 0; t < PIECE_COUNT; t++) {
        s_pieces[t][0] = s_base[t];
        s_pieces[t][1] = rot_cw(s_pieces[t][0]);
        s_pieces[t][2] = rot_cw(s_pieces[t][1]);
        s_pieces[t][3] = rot_cw(s_pieces[t][2]);
    }

    for (int c = 0; c < COLS; c++) {
        for (int r = 0; r < ROWS; r++) {
            s_grid[c][r] = 0;
        }
    }
    s_acc = 0;
    s_drop_ticks = 50;
    s_lines = 0;
    s_score = 0;
    s_over = false;
    s_hold_ticks = 0;
    s_rand = 0xA31D0u ^ (uint32_t)time_us_32();
    s_type = 0;
    s_next = (int)(prng_next() % PIECE_COUNT);
    spawn();
}

unsigned tetris_score(void)
{
    return s_score;
}

static void lock_piece(void)
{
    const uint16_t m = s_pieces[s_type][s_rot];
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            if (!(m & (1u << (r * 4 + c)))) {
                continue;
            }
            const int gx = s_px + c;
            const int gy = s_py + r;
            if (gx >= 0 && gx < COLS && gy >= 0 && gy < ROWS) {
                s_grid[gx][gy] = (uint8_t)(s_type + 1);
            }
        }
    }

    uint16_t cleared = 0;
    int write = ROWS - 1;
    for (int r = ROWS - 1; r >= 0; r--) {
        bool full = true;
        for (int c = 0; c < COLS; c++) {
            if (s_grid[c][r] == 0) {
                full = false;
                break;
            }
        }
        if (full) {
            cleared++;
            continue;
        }
        if (write != r) {
            for (int c = 0; c < COLS; c++) {
                s_grid[c][write] = s_grid[c][r];
            }
        }
        write--;
    }
    while (write >= 0) {
        for (int c = 0; c < COLS; c++) {
            s_grid[c][write] = 0;
        }
        write--;
    }

    if (cleared > 0) {
        static const uint16_t pts[4] = { 100u, 300u, 500u, 800u };
        const unsigned idx = cleared - 1 < 4 ? cleared - 1 : 3;
        s_score += pts[idx];
        s_lines = (uint16_t)(s_lines + cleared);
        const uint16_t eased = (uint16_t)(50u - (s_lines / 10u) * 2u);
        s_drop_ticks = eased < 12u ? 12u : eased;
    }
    spawn();
}

static void hard_drop(void)
{
    while (!collides(s_type, s_rot, s_px, s_py + 1)) {
        s_py++;
        s_score += 2;
    }
    lock_piece();
}

void tetris_update(void)
{
    if (s_over) {
        if (input_edge_pressed(BTN_A) || input_edge_pressed(BTN_START)) {
            tetris_init();
        }
        return;
    }

    if (input_edge_pressed(BTN_B)) {
        hard_drop();
        return;
    }

    if (input_edge_pressed(BTN_A)) {
        const int nrot = (s_rot + 1) & 3;
        const int kick[3] = { 0, -1, 1 };
        for (int k = 0; k < 3; k++) {
            if (!collides(s_type, nrot, s_px + kick[k], s_py)) {
                s_rot = nrot;
                s_px += kick[k];
                break;
            }
        }
    }

    if (input_edge_pressed(BTN_LEFT) && !collides(s_type, s_rot, s_px - 1, s_py)) {
        s_px--;
        s_hold_ticks = 0;
    } else if (input_edge_pressed(BTN_RIGHT) &&
               !collides(s_type, s_rot, s_px + 1, s_py)) {
        s_px++;
        s_hold_ticks = 0;
    } else if (input_is_pressed(BTN_LEFT) || input_is_pressed(BTN_RIGHT)) {
        s_hold_ticks++;
        if (s_hold_ticks >= 22u && (s_hold_ticks & 7u) == 0u) {
            const int dir = input_is_pressed(BTN_LEFT) ? -1 : 1;
            if (!collides(s_type, s_rot, s_px + dir, s_py)) {
                s_px += dir;
            }
        }
    }

    s_acc++;
    const uint16_t gate = input_is_pressed(BTN_DOWN) ? 4u : s_drop_ticks;
    if (s_acc >= gate) {
        s_acc = 0;
        if (!collides(s_type, s_rot, s_px, s_py + 1)) {
            s_py++;
            if (input_is_pressed(BTN_DOWN)) {
                s_score++;
            }
        } else {
            lock_piece();
        }
    }
}

static void draw_cells_mask(const uint16_t m, int x, int y, display_color_t c)
{
    for (int r = 0; r < 4; r++) {
        for (int cc = 0; cc < 4; cc++) {
            if (m & (1u << (r * 4 + cc))) {
                display_fill_rect(x + cc * CELL, y + r * CELL, CELL, CELL, c);
            }
        }
    }
}

void tetris_render(void)
{
    display_clear(DISPLAY_COLOR_BLACK);

    display_fill_rect(PF_X - 1, PF_Y - 1, COLS * CELL + 2, ROWS * CELL + 2,
                      DISPLAY_COLOR_DKGRAY);

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (s_grid[c][r] != 0) {
                const display_color_t col = s_colors[s_grid[c][r] - 1];
                display_fill_rect(PF_X + c * CELL, PF_Y + r * CELL, CELL, CELL,
                                  col);
            }
        }
    }

    if (!s_over) {
        draw_cells_mask(s_pieces[s_type][s_rot], PF_X + s_px * CELL,
                        PF_Y + s_py * CELL, s_colors[s_type]);
    }

    char buf[16];
    int n = snprintf(buf, sizeof(buf), "SCORE %05u", (unsigned)s_score);
    if (n > 0) {
        display_draw_text(2, 3, buf, DISPLAY_COLOR_WHITE, DISPLAY_COLOR_BLACK);
    }

    display_draw_text(GAP_PX + 3, 3, "NEXT", DISPLAY_COLOR_GRAY,
                      DISPLAY_COLOR_BLACK);
    const int px = GAP_PX + 3;
    const int py = 16;
    display_fill_rect(px - 1, py - 1, 4 * 4 + 2, 4 * 4 + 2,
                      DISPLAY_COLOR_DKGRAY);
    const uint16_t nm = s_pieces[s_next][0];
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (nm & (1u << (r * 4 + c))) {
                display_fill_rect(px + c * 4, py + r * 4, 4, 4,
                                  s_colors[s_next]);
            }
        }
    }

    display_draw_text(2, DISPLAY_HEIGHT - 9, "A ROT B DROP",
                      DISPLAY_COLOR_DKGRAY, DISPLAY_COLOR_BLACK);

    if (s_over) {
        display_fill_rect(38, 60, 84, 24, DISPLAY_COLOR_DKGRAY);
        display_draw_text(46, 64, "GAME OVER", DISPLAY_COLOR_RED,
                          DISPLAY_COLOR_DKGRAY);
        display_draw_text(46, 73, "PRESS A", DISPLAY_COLOR_WHITE,
                          DISPLAY_COLOR_DKGRAY);
    }
}