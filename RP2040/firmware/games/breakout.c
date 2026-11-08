/*
 * breakout.c -- Breakout for Teeny.
 *
 * 10x6 brick wall, a paddle you steer with LEFT/RIGHT, and a ball that
 * launches on A. Ball physics are fixed-point (6 fractional bits) so the 60 Hz
 * tick is deterministic; collision checks run in pixel space. Three lives,
 * score = bricks cleared, new wave on a clean board. The HUD strip on top is
 * shared layout with the other games (16 px), the border is deadly only for
 * the ball.
 */

#include "teeny/breakout.h"

#include "teeny/display.h"
#include "teeny/input.h"

#include "pico/stdlib.h"

#include <stdint.h>

#define HUD_PX       16
#define FIELD_TOP    HUD_PX
#define FIELD_BOTTOM (DISPLAY_HEIGHT - 1)          /* 143 */
#define PADDLE_W     26
#define PADDLE_H     5
#define PADDLE_Y     138
#define BALL_R       2
#define BRICK_W      15
#define BRICK_H      6
#define BRICK_DX     16
#define BRICK_DY     8
#define BRICK_ROWS   6
#define BRICK_COLS   10
#define BRICK_Y0     (FIELD_TOP + 4)
#define LIVES        3
#define MAX_WAVE     200

typedef struct {
    int32_t x, y;   /* fixed point, 6 fractional bits */
    int32_t vx, vy;
    bool    stuck;  /* resting on the paddle until A launches it */
} ball_t;

static ball_t s_ball;
static int s_paddle_x;
static bool s_bricks[BRICK_COLS][BRICK_ROWS];
static int s_wave;
static unsigned s_score;
static int s_lives;
static bool s_over;
static uint32_t s_rand;

static const display_color_t s_row_colors[BRICK_ROWS] = {
    DISPLAY_COLOR_RED, DISPLAY_COLOR_MAGENTA, DISPLAY_COLOR_YELLOW,
    DISPLAY_COLOR_GREEN, DISPLAY_COLOR_CYAN, DISPLAY_COLOR_BLUE,
};

static uint32_t prng_next(void)
{
    uint32_t x = s_rand;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s_rand = x != 0 ? x : 0xA5A5A5A5u;
    return s_rand;
}

static void reset_bricks(void)
{
    for (int c = 0; c < BRICK_COLS; c++) {
        for (int r = 0; r < BRICK_ROWS; r++) {
            s_bricks[c][r] = true;
        }
    }
}

static void stick_ball(void)
{
    s_ball.stuck = true;
    s_ball.x = (int32_t)(s_paddle_x + PADDLE_W / 2) << 6;
    s_ball.y = (int32_t)(PADDLE_Y - BALL_R - 1) << 6;
    s_ball.vx = 0;
    s_ball.vy = 0;
}

void breakout_init(void)
{
    s_paddle_x = (DISPLAY_WIDTH - PADDLE_W) / 2;
    s_wave = 1;
    s_score = 0;
    s_lives = LIVES;
    s_over = false;
    s_rand = 0xF00DBEEFu ^ (uint32_t)time_us_32();
    reset_bricks();
    stick_ball();
}

unsigned breakout_score(void)
{
    return s_score;
}

static void launch_ball(void)
{
    s_ball.stuck = false;
    /* Aim roughly up; give it a little horizontal bias so runs vary. */
    int32_t vx = ((int32_t)(prng_next() % 9) - 4);   /* -4..+4 px/...        */
    vx = (vx * 20) / 8;                              /* scale to frac units  */
    s_ball.vx = vx >> 4;                             /* ~ +/- 0.6 px/tick   */
    s_ball.vy = -(int32_t)((52 << 6) / 60);          /* ~ 0.85 px/tick up   */
}

static void clamp_paddle(void)
{
    if (s_paddle_x < 0) {
        s_paddle_x = 0;
    }
    if (s_paddle_x > DISPLAY_WIDTH - PADDLE_W) {
        s_paddle_x = DISPLAY_WIDTH - PADDLE_W;
    }
}

static void hit_bricks(int32_t bx, int32_t by)
{
    for (int c = 0; c < BRICK_COLS; c++) {
        for (int r = 0; r < BRICK_ROWS; r++) {
            if (!s_bricks[c][r]) {
                continue;
            }
            const int l = c * BRICK_DX;
            const int t = BRICK_Y0 + r * BRICK_DY;
            if (bx >= l - BALL_R && bx <= l + BRICK_W - 1 + BALL_R &&
                by >= t - BALL_R && by <= t + BRICK_H - 1 + BALL_R) {
                s_bricks[c][r] = false;
                s_score++;
                s_ball.vy = -s_ball.vy; /* bounce off the brick */
                s_ball.x += s_ball.vx;
                s_ball.y += s_ball.vy;
                return;
            }
        }
    }
}

static void wave_done(void)
{
    s_wave++;
    if (s_wave > MAX_WAVE) {
        s_wave = MAX_WAVE;
    }
    reset_bricks();
    stick_ball();
}

void breakout_update(void)
{
    if (s_over) {
        if (input_edge_pressed(BTN_A) || input_edge_pressed(BTN_START)) {
            breakout_init();
        }
        return;
    }

    if (input_is_pressed(BTN_LEFT)) {
        s_paddle_x -= 2;
    }
    if (input_is_pressed(BTN_RIGHT)) {
        s_paddle_x += 2;
    }
    clamp_paddle();

    if ((input_edge_pressed(BTN_A) || input_edge_pressed(BTN_START)) &&
        s_ball.stuck) {
        launch_ball();
    }
    if (s_ball.stuck) {
        stick_ball(); /* follow the paddle while waiting */
        return;
    }

    s_ball.x += s_ball.vx;
    s_ball.y += s_ball.vy;

    const int32_t bx = s_ball.x >> 6;
    const int32_t by = s_ball.y >> 6;

    /* Left/right/top walls. */
    if (bx <= 1) {
        s_ball.x = (int32_t)(2) << 6;
        s_ball.vx = -s_ball.vx;
    } else if (bx >= DISPLAY_WIDTH - 1) {
        s_ball.x = (int32_t)(DISPLAY_WIDTH - 2) << 6;
        s_ball.vx = -s_ball.vx;
    }
    if (by <= FIELD_TOP + 1) {
        s_ball.y = (int32_t)(FIELD_TOP + 2) << 6;
        s_ball.vy = -s_ball.vy;
    }

    /* Paddle bounce (ball travelling down, at paddle height, in reach). */
    const int32_t ball_bottom = by + BALL_R;
    if (s_ball.vy > 0 && ball_bottom >= PADDLE_Y &&
        ball_bottom <= PADDLE_Y + PADDLE_H &&
        bx >= s_paddle_x - BALL_R - 2 && bx <= s_paddle_x + PADDLE_W + BALL_R + 2) {
        s_ball.vy = -s_ball.vy;
        /* Spin: nudge horizontal velocity by which way the paddle is moving. */
        if (input_is_pressed(BTN_LEFT)) {
            s_ball.vx -= (6 << 6) / 60;
        }
        if (input_is_pressed(BTN_RIGHT)) {
            s_ball.vx += (6 << 6) / 60;
        }
        s_ball.y = (int32_t)(PADDLE_Y - BALL_R - 1) << 6;
    }

    /* Bricks (px-space overlap test). */
    hit_bricks(bx, by);

    /* Missed the ball: life lost. */
    if (by > FIELD_BOTTOM) {
        s_lives--;
        if (s_lives < 0) {
            s_over = true;
        } else {
            stick_ball();
        }
        return;
    }

    /* Bord of bricks cleared -> next wave. */
    bool any = false;
    for (int c = 0; c < BRICK_COLS && !any; c++) {
        for (int r = 0; r < BRICK_ROWS && !any; r++) {
            any = s_bricks[c][r];
        }
    }
    if (!any) {
        wave_done();
    }
}

static void draw_hud(void)
{
    char hud[10] = "SCORE 000";
    hud[6] = (char)('0' + (s_score / 100) % 10);
    hud[7] = (char)('0' + (s_score / 10) % 10);
    hud[8] = (char)('0' + (s_score % 10));
    display_draw_text(4, 4, hud, DISPLAY_COLOR_WHITE, DISPLAY_COLOR_BLACK);

    char wave[10] = "WAVE 0";
    wave[5] = (char)('0' + s_wave % 10);
    display_draw_text(96, 4, wave, DISPLAY_COLOR_CYAN, DISPLAY_COLOR_BLACK);

    for (int i = 0; i < LIVES; i++) {
        display_fill_rect(144, 5 + i * 3, 4, 2,
                          i < s_lives ? DISPLAY_COLOR_RED : DISPLAY_COLOR_DKGRAY);
    }
}

void breakout_render(void)
{
    display_clear(DISPLAY_COLOR_BLACK);
    draw_hud();

    display_fill_rect(0, FIELD_TOP - 1, DISPLAY_WIDTH, 1, DISPLAY_COLOR_DKGRAY);

    for (int c = 0; c < BRICK_COLS; c++) {
        for (int r = 0; r < BRICK_ROWS; r++) {
            if (s_bricks[c][r]) {
                display_fill_rect(c * BRICK_DX, BRICK_Y0 + r * BRICK_DY,
                                  BRICK_W, BRICK_H, s_row_colors[r]);
            }
        }
    }

    display_fill_rect(s_paddle_x, PADDLE_Y, PADDLE_W, PADDLE_H,
                      DISPLAY_COLOR_WHITE);

    const int bx = (int)(s_ball.x >> 6);
    const int by = (int)(s_ball.y >> 6);
    display_fill_rect(bx - BALL_R, by - BALL_R, BALL_R * 2 + 1, BALL_R * 2 + 1,
                      DISPLAY_COLOR_YELLOW);

    if (s_over) {
        display_fill_rect(36, 56, 88, 32, DISPLAY_COLOR_BLACK);
        display_draw_text(46, 64, "GAME OVER", DISPLAY_COLOR_WHITE,
                          DISPLAY_COLOR_BLACK);
        display_draw_text(53, 76, "PRESS A", DISPLAY_COLOR_YELLOW,
                          DISPLAY_COLOR_BLACK);
    }
}