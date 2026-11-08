/*
 * flappy.c -- FLAPPY, a tiny bird / pipe-runner for Teeny.
 *
 * A one-button flapper in the *Flappy* tradition: the bird is gravity-fed, A
 * (or B / UP) gives a flap, the gap between the scrolling green pipes slowly
 * squeezes together and the scenery speeds up as your score climbs. Fixed
 * point (6 fractional bits) keeps the 60 Hz tick deterministic.
 */

#include "teeny/flappy.h"

#include "teeny/display.h"
#include "teeny/input.h"

#include "pico/stdlib.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define BIRD_X      30
#define BIRD_W      7
#define BIRD_H      7
#define GROUND_Y    132
#define GRAV        (int)0x18      /* 0.375 px/tick^2 */
#define FLAPV       (int)0x110     /* -4.25 px/tick (up) */
#define MAX_FALL    (int)0xC0      /* 3 px/tick (down) */
#define MAX_TUBES   5
#define TUBE_W      8
#define VIEW_GAP    2              /* pipe x reserved for keep-away gap */
#define BASE_GAP    54

static unsigned s_score;
static int      s_y;               /* fixed bird top */
static int      s_vy;
static bool     s_over;
static int      s_speed;
static int      s_gap;

typedef struct {
    int  x;                        /* left edge of the pipe */
    int  gap_y;                    /* pixel row of the gap top */
    int  gap_h;                    /* gap height in pixels */
    bool scored;
} tube_t;

static tube_t s_tubes[MAX_TUBES];
static int    s_tube_count;
static uint16_t s_spawn_cd;
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

static void spawn_tube(void)
{
    if (s_tube_count >= MAX_TUBES) {
        return;
    }
    tube_t *t = &s_tubes[s_tube_count];
    t->x = DISPLAY_WIDTH + VIEW_GAP;
    t->gap_h = s_gap;
    const int lo = 22;
    const int hi = GROUND_Y - 4 - s_gap;
    t->gap_y = lo + (int)(prng_next() % (uint32_t)(hi - lo));
    t->scored = false;
    s_tube_count++;
}

void flappy_init(void)
{
    s_score = 0;
    s_y = (int)((GROUND_Y - BIRD_H) / 2) << 6;
    s_vy = 0;
    s_over = false;
    s_speed = 1;
    s_gap = BASE_GAP;
    s_tube_count = 0;
    s_spawn_cd = 40;
    s_rand = 0xFAF0u ^ (uint32_t)time_us_32();
    spawn_tube();
}

unsigned flappy_score(void)
{
    return s_score;
}

static bool bird_hits_tube(const tube_t *t)
{
    const int bx = BIRD_X, by = s_y >> 6;
    if (bx + BIRD_W <= t->x || bx >= t->x + TUBE_W) {
        return false;
    }
    if (by < t->gap_y) {
        return true;               /* top half */
    }
    if (by + BIRD_H > t->gap_y + t->gap_h) {
        return true;               /* bottom half */
    }
    return false;
}

void flappy_update(void)
{
    if (s_over) {
        if (input_edge_pressed(BTN_A) || input_edge_pressed(BTN_START)) {
            flappy_init();
        }
        return;
    }

    if (input_edge_pressed(BTN_A) || input_edge_pressed(BTN_B) ||
        input_edge_pressed(BTN_UP)) {
        s_vy = -FLAPV;
    }
    s_vy += GRAV;
    if (s_vy > MAX_FALL) {
        s_vy = MAX_FALL;
    }
    s_y += s_vy;

    for (int i = 0; i < s_tube_count; i++) {
        tube_t *t = &s_tubes[i];
        t->x -= s_speed;
        if (!t->scored && t->x + TUBE_W < BIRD_X) {
            t->scored = true;
            s_score++;
            if (s_score % 8u == 0u) {
                if (s_gap > 40) {
                    s_gap -= 2;
                }
                if (s_speed < 2) {
                    s_speed++;
                }
            }
        }
    }
    while (s_tube_count > 0 && s_tubes[0].x + TUBE_W < 0) {
        for (int i = 1; i < s_tube_count; i++) {
            s_tubes[i - 1] = s_tubes[i];
        }
        s_tube_count--;
    }

    if (s_spawn_cd > 0) {
        s_spawn_cd--;
    } else if (s_tube_count == 0 || s_tubes[s_tube_count - 1].x < 78) {
        spawn_tube();
        s_spawn_cd = 46;
    }

    const int by = s_y >> 6;
    if (by < 0) {
        s_y = 0;
        s_vy = 0;
    }
    if (by + BIRD_H >= GROUND_Y) {
        s_over = true;
        return;
    }
    for (int i = 0; i < s_tube_count; i++) {
        if (bird_hits_tube(&s_tubes[i])) {
            s_over = true;
            return;
        }
    }
}

static void draw_tube(const tube_t *t)
{
    const int x = t->x;
    display_fill_rect(x, 0, TUBE_W, t->gap_y, DISPLAY_COLOR_GREEN);
    display_fill_rect(x, t->gap_y + t->gap_h, TUBE_W,
                      GROUND_Y - (t->gap_y + t->gap_h), DISPLAY_COLOR_GREEN);
    display_fill_rect(x + 1, 0, TUBE_W - 2, 2, DISPLAY_COLOR_WHITE);
    display_fill_rect(x, t->gap_y - 3, TUBE_W, 3, DISPLAY_COLOR(0, 130, 0));
    display_fill_rect(x, t->gap_y + t->gap_h, TUBE_W, 3,
                      DISPLAY_COLOR(0, 130, 0));
}

void flappy_render(void)
{
    display_clear(DISPLAY_COLOR_BLACK);

    for (int i = 0; i < s_tube_count; i++) {
        draw_tube(&s_tubes[i]);
    }

    /* Ground: grass lip over packed dirt. */
    display_fill_rect(0, GROUND_Y, DISPLAY_WIDTH, 4, DISPLAY_COLOR_GREEN);
    display_fill_rect(0, GROUND_Y + 4, DISPLAY_WIDTH,
                      DISPLAY_HEIGHT - GROUND_Y - 4, DISPLAY_COLOR(96, 56, 16));

    /* Bird. */
    const int by = s_y >> 6;
    display_fill_rect(BIRD_X, by, BIRD_W, BIRD_H, DISPLAY_COLOR_WHITE);
    display_fill_rect(BIRD_X + BIRD_W, by + 2, 2, 2, DISPLAY_COLOR(255, 150, 0));
    display_fill_rect(BIRD_X + 1, by + 1, 2, 2, DISPLAY_COLOR_BLACK);
    if (s_vy < 0) {
        display_fill_rect(BIRD_X + 1, by + 5, BIRD_W - 2, 1,
                          DISPLAY_COLOR_GRAY);
    } else {
        display_fill_rect(BIRD_X + 1, by + 1, BIRD_W - 2, 1,
                          DISPLAY_COLOR_GRAY);
    }

    char buf[16];
    int n = snprintf(buf, sizeof(buf), "%u", (unsigned)s_score);
    if (n > 0) {
        display_draw_text(DISPLAY_WIDTH / 2 - 6, 6, buf, DISPLAY_COLOR_WHITE,
                          DISPLAY_COLOR_BLACK);
    }

    if (s_over) {
        display_fill_rect(38, 60, 84, 24, DISPLAY_COLOR_DKGRAY);
        display_draw_text(46, 64, "GAME OVER", DISPLAY_COLOR_RED,
                          DISPLAY_COLOR_DKGRAY);
        display_draw_text(53, 73, "A FLAP", DISPLAY_COLOR_WHITE,
                          DISPLAY_COLOR_DKGRAY);
    }
}