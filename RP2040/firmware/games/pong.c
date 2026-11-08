/*
 * pong.c -- Pong for Teeny.
 *
 * One player versus a CPU paddle on the right. UP/DOWN steer your paddle, A
 * serves, first to 7 wins. Ball physics are fixed-point (6 fractional bits)
 * so the 60 Hz tick is deterministic; the bounce angle depends on where the
 * ball meets the paddle. A dashed center line gives the retro court look.
 */

#include "teeny/pong.h"

#include "teeny/display.h"
#include "teeny/input.h"

#include "pico/stdlib.h"

#include <stdint.h>
#include <stdio.h>

#define HUD_PX      16
#define PAD_W       4
#define PAD_H       26
#define PAD_X       6
#define CPU_X       (DISPLAY_WIDTH - 6 - PAD_W)
#define BALL_R      3
#define WIN_SCORE   7
#define SERVE_TICKS 40

static int32_t s_ball_x, s_ball_y;  /* fixed point, 6 fractional bits */
static int32_t s_ball_vx, s_ball_vy;
static int s_ball_stuck;
static int s_px;                    /* player paddle top y */
static int s_cpy;                   /* cpu paddle top y */
static int s_serve;                 /* >0 while a point is on serve */
static unsigned s_us;
static unsigned s_cpu;
static bool s_over;
static uint32_t s_rand;

#define FIX (1 << 6)

static uint32_t prng_next(void)
{
    uint32_t x = s_rand;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s_rand = x != 0 ? x : 0xA5A5A5A5u;
    return s_rand;
}

static void clamp_paddle_y(int *y)
{
    if (*y < 0) {
        *y = 0;
    }
    if (*y > DISPLAY_HEIGHT - PAD_H) {
        *y = DISPLAY_HEIGHT - PAD_H;
    }
}

static void center_ball(void)
{
    s_ball_x = (int32_t)(DISPLAY_WIDTH / 2) << 6;
    s_ball_y = (int32_t)(DISPLAY_HEIGHT / 2) << 6;
    s_ball_vx = 0;
    s_ball_vy = 0;
    s_ball_stuck = true;
}

void pong_init(void)
{
    s_px = (DISPLAY_HEIGHT - PAD_H) / 2;
    s_cpy = s_px;
    s_us = 0;
    s_cpu = 0;
    s_over = false;
    s_rand = 0xB16B00u ^ (uint32_t)time_us_32();
    center_ball();
    s_serve = SERVE_TICKS;
}

unsigned pong_score(void)
{
    return s_us;
}

static void serve(void)
{
    /* Serve toward whoever just lost the point (first point: the CPU). */
    const bool to_cpu = (s_us + s_cpu) == 0 || (s_us + s_cpu) % 2 != 0;
    const int32_t vx = to_cpu ? 0x33 : -0x33;
    const int32_t vy = (int32_t)((int)(prng_next() % 9) - 4) * 4;
    s_ball_vx = vx;
    s_ball_vy = vy;
    s_ball_stuck = false;
    s_serve = 0;
}

static void award(int player)
{
    if (player == 0) {
        s_us++;
    } else {
        s_cpu++;
    }
    if (s_us >= WIN_SCORE || s_cpu >= WIN_SCORE) {
        s_over = true;
        return;
    }
    center_ball();
    s_serve = SERVE_TICKS;
}

void pong_update(void)
{
    if (s_over) {
        if (input_edge_pressed(BTN_A) || input_edge_pressed(BTN_START)) {
            pong_init();
        }
        return;
    }

    if (s_ball_stuck) {
        /* Serve whenever the player asks -- during the READY countdown or
         * any time afterwards. The ball never sits dead. */
        if (input_edge_pressed(BTN_A) || input_edge_pressed(BTN_START)) {
            serve();
        } else if (s_serve > 0) {
            s_serve--;
        }
        return;
    }

    if (input_is_pressed(BTN_UP)) {
        s_px -= 2;
    }
    if (input_is_pressed(BTN_DOWN)) {
        s_px += 2;
    }
    clamp_paddle_y(&s_px);

    if (s_ball_vx > 0) {
        const int32_t target = (s_ball_y >> 6) - PAD_H / 2;
        if (s_cpy < target - 3) {
            s_cpy += 2;
        } else if (s_cpy > target + 3) {
            s_cpy -= 2;
        }
    }
    clamp_paddle_y(&s_cpy);

    s_ball_x += s_ball_vx;
    s_ball_y += s_ball_vy;

    if ((s_ball_y >> 6) - BALL_R <= 0) {
        s_ball_y = (int32_t)(BALL_R) << 6;
        s_ball_vy = -s_ball_vy;
    }
    if ((s_ball_y >> 6) + BALL_R >= DISPLAY_HEIGHT) {
        s_ball_y = (int32_t)(DISPLAY_HEIGHT - BALL_R) << 6;
        s_ball_vy = -s_ball_vy;
    }

    const int32_t bx = s_ball_x >> 6;
    const int32_t by = s_ball_y >> 6;

    if (s_ball_vx < 0 && bx - BALL_R <= PAD_X + PAD_W && bx + BALL_R >= PAD_X &&
        by + BALL_R >= s_px && by - BALL_R <= s_px + PAD_H) {
        const int32_t mid = s_px + PAD_H / 2;
        s_ball_vy = (by - mid) * 2;
        if (s_ball_vy > 0x20) {
            s_ball_vy = 0x20;
        }
        if (s_ball_vy < -0x20) {
            s_ball_vy = -0x20;
        }
        s_ball_vx = 0x33;
        s_ball_x = (int32_t)(PAD_X + PAD_W + BALL_R) << 6;
    } else if (s_ball_vx > 0 && bx + BALL_R >= CPU_X &&
               bx - BALL_R <= CPU_X + PAD_W && by + BALL_R >= s_cpy &&
               by - BALL_R <= s_cpy + PAD_H) {
        const int32_t mid = s_cpy + PAD_H / 2;
        s_ball_vy = (by - mid) * 2;
        if (s_ball_vy > 0x20) {
            s_ball_vy = 0x20;
        }
        if (s_ball_vy < -0x20) {
            s_ball_vy = -0x20;
        }
        s_ball_vx = -0x33;
        s_ball_x = (int32_t)(CPU_X - BALL_R) << 6;
    }

    if (bx + BALL_R < 0) {
        award(1);
    } else if (bx - BALL_R > DISPLAY_WIDTH) {
        award(0);
    }
}

void pong_render(void)
{
    display_clear(DISPLAY_COLOR_BLACK);

    for (int i = 0; i < DISPLAY_HEIGHT; i += 10) {
        display_fill_rect(DISPLAY_WIDTH / 2 - 1, i, 2, 4, DISPLAY_COLOR_DKGRAY);
    }

    display_fill_rect(PAD_X, s_px, PAD_W, PAD_H, DISPLAY_COLOR_WHITE);
    display_fill_rect(CPU_X, s_cpy, PAD_W, PAD_H, DISPLAY_COLOR_WHITE);
    display_fill_rect((int)(s_ball_x >> 6) - BALL_R,
                      (int)(s_ball_y >> 6) - BALL_R, BALL_R * 2 + 1,
                      BALL_R * 2 + 1, DISPLAY_COLOR_WHITE);

    char buf[16];
    int n = snprintf(buf, sizeof(buf), "US %02u", (unsigned)s_us);
    if (n > 0) {
        display_draw_text(20, 3, buf, DISPLAY_COLOR_GREEN, DISPLAY_COLOR_BLACK);
    }
    n = snprintf(buf, sizeof(buf), "CPU %02u", (unsigned)s_cpu);
    if (n > 0) {
        display_draw_text(DISPLAY_WIDTH - 60, 3, buf, DISPLAY_COLOR_MAGENTA,
                          DISPLAY_COLOR_BLACK);
    }

    display_draw_text(2, DISPLAY_HEIGHT - 9, "UP DOWN A",
                      DISPLAY_COLOR_DKGRAY, DISPLAY_COLOR_BLACK);

    if (s_serve > 0) {
        display_draw_text((DISPLAY_WIDTH - 5 * 6) / 2, DISPLAY_HEIGHT / 2 - 38,
                          "READY", DISPLAY_COLOR_YELLOW, DISPLAY_COLOR_BLACK);
        display_draw_text((DISPLAY_WIDTH - 7 * 6) / 2, DISPLAY_HEIGHT / 2 + 16,
                          "PRESS A", DISPLAY_COLOR_DKGRAY, DISPLAY_COLOR_BLACK);
    }

    if (s_over) {
        display_fill_rect(40, 60, 80, 24, DISPLAY_COLOR_DKGRAY);
        display_draw_text(48, 64, s_us >= WIN_SCORE ? "YOU WIN" : "CPU WINS",
                          s_us >= WIN_SCORE ? DISPLAY_COLOR_GREEN
                                            : DISPLAY_COLOR_RED,
                          DISPLAY_COLOR_DKGRAY);
        display_draw_text(46, 73, "PRESS A", DISPLAY_COLOR_WHITE,
                          DISPLAY_COLOR_DKGRAY);
    }
}