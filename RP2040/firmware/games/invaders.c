/*
 * invaders.c -- Space Invaders for Teeny.
 *
 * A player turret at the bottom that strafes with LEFT/RIGHT and fires on A
 * (rate-limited). A marching grid of invaders sweeps left-right and descends
 * on each edge; you lose when the front row reaches your line. Clear the
 * grid for a new, faster wave. Scoped out on purpose: no invader fire, no
 * sprites -- rects + the 5x7 font only, matching the Phase-3 graphics API.
 */

#include "teeny/invaders.h"

#include "teeny/display.h"
#include "teeny/input.h"

#include "pico/stdlib.h"

#include <stdint.h>

#define HUD_PX        16
#define PLAYER_W      14
#define PLAYER_H      6
#define PLAYER_Y      134
#define PLAYER_SPEED  2
#define INV_W         12
#define INV_H         8
#define INV_DX        16
#define INV_DY        12
#define INV_COLS      8
#define INV_ROWS      5
#define INV_X0        24
#define INV_Y0        (HUD_PX + 8)
#define BULLET_W      2
#define BULLET_H      6
#define BULLET_SPEED  2

typedef struct {
    int16_t x, y;   /* pixel top-left; y = -1 when inactive */
} bullet_t;

static int s_player_x;
static bullet_t s_bullet;
static bool s_inv[INV_COLS][INV_ROWS];
static int  s_gx, s_gy;          /* grid origin (top-left of the field) */
static int  s_dir;               /* +1 right, -1 left */
static uint8_t s_step_acc;
static uint8_t s_move_interval;
static uint8_t s_fire_cooldown;
static unsigned s_score;
static unsigned s_wave;
static bool s_over;

void invaders_init(void)
{
    s_player_x = (DISPLAY_WIDTH - PLAYER_W) / 2;
    s_bullet.x = 0;
    s_bullet.y = -1;
    s_dir = 1;
    s_wave = 1;
    s_score = 0;
    s_over = false;
    s_step_acc = 0;
    s_fire_cooldown = 0;
    s_move_interval = 40;

    s_gx = INV_X0;
    s_gy = INV_Y0;
    for (int c = 0; c < INV_COLS; c++) {
        for (int r = 0; r < INV_ROWS; r++) {
            s_inv[c][r] = true;
        }
    }
}

unsigned invaders_score(void)
{
    return s_score;
}

static void fire(void)
{
    if (s_bullet.y >= 0 || s_fire_cooldown > 0) {
        return;
    }
    s_bullet.x = s_player_x + PLAYER_W / 2 - BULLET_W / 2;
    s_bullet.y = PLAYER_Y - BULLET_H;
    s_fire_cooldown = 28; /* ticks between shots */
}

static int inv_extent(int *min_out, int *max_out)
{
    int minx = INT32_MAX;
    int maxx = INT32_MIN;
    int alive = 0;
    for (int c = 0; c < INV_COLS; c++) {
        for (int r = 0; r < INV_ROWS; r++) {
            if (!s_inv[c][r]) {
                continue;
            }
            alive++;
            int x = s_gx + c * INV_DX;
            if (x < minx) { minx = x; }
            if (x + INV_W > maxx) { maxx = x + INV_W; }
        }
    }
    if (alive == 0) {
        return 0;
    }
    *min_out = minx;
    *max_out = maxx;
    return alive;
}

static void respawn_wave(void)
{
    s_wave++;
    s_gx = INV_X0;
    s_gy = INV_Y0;
    s_dir = 1;
    for (int c = 0; c < INV_COLS; c++) {
        for (int r = 0; r < INV_ROWS; r++) {
            s_inv[c][r] = true;
        }
    }
    const unsigned interval = 40u - (s_wave - 1) * 3u;
    s_move_interval = interval > 16u ? (uint8_t)interval : 16u;
}

void invaders_update(void)
{
    if (s_over) {
        if (input_edge_pressed(BTN_A) || input_edge_pressed(BTN_START)) {
            invaders_init();
        }
        return;
    }

    if (input_is_pressed(BTN_LEFT)) {
        s_player_x -= PLAYER_SPEED;
    }
    if (input_is_pressed(BTN_RIGHT)) {
        s_player_x += PLAYER_SPEED;
    }
    if (s_player_x < 0) {
        s_player_x = 0;
    }
    if (s_player_x > DISPLAY_WIDTH - PLAYER_W) {
        s_player_x = DISPLAY_WIDTH - PLAYER_W;
    }

    if (input_edge_pressed(BTN_A)) {
        fire();
    }

    /* Bullet: rise, and check the whole alive grid. */
    if (s_bullet.y >= 0) {
        s_bullet.y -= BULLET_SPEED;
        if (s_bullet.y < HUD_PX) {
            s_bullet.y = -1;
        } else {
            const int bl = s_bullet.x;
            const int br = s_bullet.x + BULLET_W - 1;
            const int bt = s_bullet.y;
            const int bb = s_bullet.y + BULLET_H - 1;
            bool hit = false;
            for (int c = 0; c < INV_COLS && !hit; c++) {
                for (int r = 0; r < INV_ROWS && !hit; r++) {
                    if (!s_inv[c][r]) {
                        continue;
                    }
                    const int l = s_gx + c * INV_DX;
                    const int t = s_gy + r * INV_DY;
                    if (br >= l && bl <= l + INV_W - 1 && bb >= t && bt <= t + INV_H - 1) {
                        s_inv[c][r] = false;
                        s_score += 10;
                        s_bullet.y = -1;
                        hit = true;
                    }
                }
            }
        }
    }
    if (s_fire_cooldown > 0) {
        s_fire_cooldown--;
    }

    if (++s_step_acc >= s_move_interval) {
        s_step_acc = 0;
        int minx, maxx;
        if (inv_extent(&minx, &maxx) > 0) {
            if (s_dir > 0 && maxx + 1 >= DISPLAY_WIDTH) {
                s_dir = -1;
                s_gy += 8;
            } else if (s_dir < 0 && minx - 1 <= 0) {
                s_dir = 1;
                s_gy += 8;
            } else {
                s_gx += s_dir * 6;
            }

            /* Front row reached the player's line? */
            int bottom_most = -1;
            for (int r = 0; r < INV_ROWS; r++) {
                for (int c = 0; c < INV_COLS; c++) {
                    if (s_inv[c][r] && (int)r > bottom_most) {
                        bottom_most = r;
                    }
                }
            }
            if (bottom_most >= 0 && s_gy + bottom_most * INV_DY + INV_H >= PLAYER_Y) {
                s_over = true;
            }
        } else {
            respawn_wave();
        }
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
}

void invaders_render(void)
{
    display_clear(DISPLAY_COLOR_BLACK);
    draw_hud();

    for (int c = 0; c < INV_COLS; c++) {
        for (int r = 0; r < INV_ROWS; r++) {
            if (!s_inv[c][r]) {
                continue;
            }
            const int x = s_gx + c * INV_DX;
            const int y = s_gy + r * INV_DY;
            display_fill_rect(x + 2, y, INV_W - 4, 3, DISPLAY_COLOR_GREEN);
            display_fill_rect(x, y + 1, INV_W, 5, DISPLAY_COLOR_GREEN);
            display_fill_rect(x + 3, y + 6, INV_W - 6, 2, DISPLAY_COLOR_GREEN);
        }
    }

    if (s_bullet.y >= 0) {
        display_fill_rect(s_bullet.x, s_bullet.y, BULLET_W, BULLET_H,
                          DISPLAY_COLOR_WHITE);
    }

    /* Player turret. */
    display_fill_rect(s_player_x, PLAYER_Y, PLAYER_W, PLAYER_H, DISPLAY_COLOR_WHITE);
    display_fill_rect(s_player_x + 4, PLAYER_Y - 2, 6, 2, DISPLAY_COLOR_WHITE);

    if (s_over) {
        display_fill_rect(36, 56, 88, 32, DISPLAY_COLOR_BLACK);
        display_draw_text(46, 64, "GAME OVER", DISPLAY_COLOR_WHITE,
                          DISPLAY_COLOR_BLACK);
        display_draw_text(53, 76, "PRESS A", DISPLAY_COLOR_YELLOW,
                          DISPLAY_COLOR_BLACK);
    }
}