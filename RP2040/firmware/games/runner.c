/*
 * runner.c -- RUNNER, a small sidescrolling platformer for Teeny.
 *
 * A tile world (8px tiles, ~96 columns wide) scrolls behind a tiny hero.
 * Jump with A (or B / UP); steer with LEFT/RIGHT. Grab coins, bounce on the
 * gummy enemies, hop the pits, and reach the flag catch to advance to the
 * next procedural level (wider pits, more platforms and grunts per level).
 * Falls and enemy bumps cost a life; losing all three (3 total) ends the run.
 * Physics are fixed point (6 fractional bits) so the 60 Hz tick is
 * deterministic.
 */

#include "teeny/runner.h"

#include "teeny/display.h"
#include "teeny/input.h"

#include "pico/stdlib.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define TILE        8
#define WORLD_COLS  96
#define TILES_Y     (DISPLAY_HEIGHT / TILE)  /* 18 */
#define GROUND        4                      /* default ground height (tiles) */
#define PLAT_MIN      6
#define PLAT_MAX     11
#define P_W           8                      /* hero hitbox */
#define P_H          12
#define GRAV          (int)0x05              /* 0.078 px/tick^2 */
#define RUN_SPD      (int)0x58               /* 1.375 px/tick */
#define JUMPV        (int)0xD8               /* -3.375 px/tick (up) */
#define MAX_FALL     (int)0x80               /* 2.0 px/tick (down) */
#define STOMP_BOUNCE (int)0xA0
#define MAX_ENEMIES  10
#define LIVES        3

enum {
    RUN_RUNNING = 0,
    RUN_WON     = 1,
    RUN_DEAD    = 2,
};

static const display_color_t C_GRASS = DISPLAY_COLOR(0, 170, 0);
static const display_color_t C_DIRT  = DISPLAY_COLOR(96, 56, 16);

typedef struct {
    uint8_t ground;      /* ground height in tiles, 0 = pit */
    int8_t  plat;        /* row of a floating platform, -1 = none */
    uint8_t coin;        /* coin present */
    uint8_t enemy;       /* grunt spawns here */
} col_t;

typedef struct {
    int px, py, vy;      /* fixed point, 6 fractional bits */
    bool on_ground;
    int dir;             /* facing: -1 / +1 */
} hero_t;

typedef struct {
    int x, y, vy;        /* x,y fixed point */
    int dir;
    int home;            /* home column of the walker */
    int travel;          /* px walked since it turned */
    bool on_ground;
    bool alive;
} grunt_t;

static col_t   s_col[WORLD_COLS];
static grunt_t s_grunts[MAX_ENEMIES];
static int     s_grunt_count;
static hero_t  s_hero;
static uint16_t s_level;
static unsigned s_score;
static int      s_lives;
static int      s_state;
static uint16_t s_state_ticks;
static int      s_cam;
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

static void gen_level(void)
{
    const int lvl = s_level - 1;
    for (int c = 0; c < WORLD_COLS; c++) {
        s_col[c].ground = GROUND;
        s_col[c].plat = -1;
        s_col[c].coin = 0;
        s_col[c].enemy = 0;
    }

    /* Pit gaps -- never in the first or last columns, never wider than 2. */
    int c = 16;
    while (c < WORLD_COLS - 8) {
        const int base = 4 + lvl * 2;
        const int pit_pct = base > 13 ? 13 : base;
        if ((int)(prng_next() % 100) < pit_pct && s_col[c].ground > 0) {
            const int len = 1 + (int)(prng_next() % 2);
            for (int i = 0; i < len && c < WORLD_COLS - 8; i++) {
                s_col[c].ground = 0;
                c++;
            }
        }
        c += 4 + (int)(prng_next() % 5);
    }

    /* Floating platforms over solid ground, 2-3 columns wide. */
    const int plat_pct = 8 + lvl * 3 > 24 ? 24 : 8 + lvl * 3;
    int p = 12;
    while (p < WORLD_COLS - 10) {
        if (s_col[p].ground > 0 && (int)(prng_next() % 100) < plat_pct) {
            const int w = 2 + (int)(prng_next() % 2);
            const int row = PLAT_MIN + (int)(prng_next() % (PLAT_MAX - PLAT_MIN));
            for (int i = 0; i < w && p + i < WORLD_COLS - 6; i++) {
                if (s_col[p + i].plat < 0) {
                    s_col[p + i].plat = (int8_t)row;
                }
            }
            p += w + 3;
        } else {
            p++;
        }
    }

    /* Coins: above platforms when present, otherwise riding the ground. */
    for (int cc = 8; cc < WORLD_COLS - 4; cc++) {
        const int pct = 16;
        if ((int)(prng_next() % 100) < pct &&
            (s_col[cc].ground > 0 || s_col[cc].plat >= 0)) {
            s_col[cc].coin = 1;
        }
    }

    /* Grunts: walkers on solid ground, spacing shrinks per level. */
    s_grunt_count = 0;
    const int step = 7 - lvl / 2 > 3 ? 7 - lvl / 2 : 3;
    int g = 20;
    while (g < WORLD_COLS - 8 && s_grunt_count < MAX_ENEMIES) {
        if (s_col[g].ground > 0 && s_col[g].plat < 0) {
            s_col[g].enemy = 1;
            grunt_t *gr = &s_grunts[s_grunt_count++];
            gr->x = (g * TILE + 2) << 6;
            gr->y = ((TILES_Y - s_col[g].ground) * TILE - 8) << 6;
            gr->vy = 0;
            gr->dir = (prng_next() & 1u) ? 1 : -1;
            gr->home = g;
            gr->travel = 30;
            gr->on_ground = true;
            gr->alive = true;
            g += step;
        } else {
            g++;
        }
    }
}

static bool tile_solid(int col, int row)
{
    if (col < 0 || col >= WORLD_COLS) {
        return true;
    }
    const col_t *cc = &s_col[col];
    if (cc->ground > 0 && row >= TILES_Y - cc->ground) {
        return true;
    }
    if (cc->plat >= 0 && row == cc->plat) {
        return true;
    }
    return false;
}

static bool box_collides(int x, int y, int w, int h)
{
    const int r0 = y / TILE, r1 = (y + h - 1) / TILE;
    const int c0 = x / TILE, c1 = (x + w - 1) / TILE;
    for (int r = r0; r <= r1; r++) {
        for (int cc = c0; cc <= c1; cc++) {
            if (tile_solid(cc, r)) {
                return true;
            }
        }
    }
    return false;
}

static int surface_row(int col)
{
    const col_t *cc = &s_col[col];
    int best = TILES_Y; /* lakeside default handled by caller */
    if (cc->ground > 0) {
        best = TILES_Y - cc->ground;
    }
    if (cc->plat >= 0 && cc->plat < best) {
        best = cc->plat;
    }
    return best;
}

void runner_init(void)
{
    s_level = 1;
    s_score = 0;
    s_lives = LIVES;
    s_state = RUN_RUNNING;
    s_state_ticks = 0;
    s_rand = 0x5EEDu ^ (uint32_t)time_us_32();
    gen_level();
    s_hero.px = (TILE * 1) << 6;
    s_hero.py = ((TILES_Y - GROUND) * TILE - P_H) << 6;
    s_hero.vy = 0;
    s_hero.on_ground = true;
    s_hero.dir = 1;
    s_cam = 0;
}

unsigned runner_score(void)
{
    return s_score;
}

static void player_dies(void)
{
    s_lives--;
    if (s_lives <= 0) {
        s_state = RUN_DEAD;
        s_state_ticks = 0;
    } else {
        s_hero.px = (TILE * 1) << 6;
        s_hero.py = ((TILES_Y - GROUND) * TILE - P_H) << 6;
        s_hero.vy = 0;
        s_hero.on_ground = true;
        s_cam = 0;
    }
}

void runner_update(void)
{
    if (s_state == RUN_WON) {
        s_state_ticks++;
        if (s_state_ticks > 90) {
            s_level++;
            s_score += 100u * s_level;
            gen_level();
            s_hero.px = (TILE * 1) << 6;
            s_hero.py = ((TILES_Y - GROUND) * TILE - P_H) << 6;
            s_hero.vy = 0;
            s_hero.on_ground = true;
            s_hero.dir = 1;
            s_cam = 0;
            s_state = RUN_RUNNING;
            s_state_ticks = 0;
        }
        return;
    }
    if (s_state == RUN_DEAD) {
        if (input_edge_pressed(BTN_A) || input_edge_pressed(BTN_START)) {
            runner_init();
        }
        return;
    }

    const int dir =
        (input_is_pressed(BTN_RIGHT) ? 1 : 0) -
        (input_is_pressed(BTN_LEFT) ? 1 : 0);
    if (dir != 0) {
        s_hero.dir = dir;
    }

    if (input_is_pressed(BTN_A) || input_is_pressed(BTN_B) ||
        input_is_pressed(BTN_UP)) {
        if (s_hero.on_ground) {
            s_hero.vy = -JUMPV;
            s_hero.on_ground = false;
        }
    }

    /* Scroll the camera to keep the hero centered-ish. */
    {
        const int world_px = WORLD_COLS * TILE - DISPLAY_WIDTH;
        s_cam = (s_hero.px >> 6) - DISPLAY_WIDTH / 3;
        if (s_cam < 0) {
            s_cam = 0;
        }
        if (s_cam > world_px) {
            s_cam = world_px;
        }
    }

    /* Horizontal motion, resolved against tiles. */
    s_hero.px += dir * RUN_SPD;
    if (dir != 0) {
        const int nx = s_hero.px >> 6;
        const int ny = s_hero.py >> 6;
        if (box_collides(nx, ny, P_W, P_H)) {
            s_hero.px -= dir * RUN_SPD;
        }
    }

    /* Gravity + vertical motion. */
    if (!s_hero.on_ground) {
        s_hero.vy += GRAV;
        if (s_hero.vy > MAX_FALL) {
            s_hero.vy = MAX_FALL;
        }
    }
    s_hero.py += s_hero.vy;
    {
        const int nx = s_hero.px >> 6;
        int ny = s_hero.py >> 6;
        if (box_collides(nx, ny, P_W, P_H)) {
            if (s_hero.vy > 0) {
                const int row = (ny + P_H - 1) / TILE;
                s_hero.py = (row * TILE - P_H) << 6;
                s_hero.vy = 0;
                s_hero.on_ground = true;
            } else {
                const int row = ny / TILE;
                s_hero.py = ((row + 1) * TILE) << 6;
                s_hero.vy = 0;
            }
        } else {
            s_hero.on_ground = false;
        }
    }

    /* Fell into a pit. */
    if (s_hero.py >> 6 >= DISPLAY_HEIGHT) {
        player_dies();
        return;
    }

    /* Coins. */
    {
        const int hx = s_hero.px >> 6, hy = s_hero.py >> 6;
        for (int cc = (hx / TILE) - 1; cc <= (hx + P_W) / TILE; cc++) {
            if (cc < 0 || cc >= WORLD_COLS || !s_col[cc].coin) {
                continue;
            }
            const int row = surface_row(cc) - 1;
            const int cx = cc * TILE + 3, cy = row * TILE + 2;
            if (hx < cx + 5 && hx + P_W > cx && hy < cy + 8 &&
                hy + P_H > cy) {
                s_col[cc].coin = 0;
                s_score += 5;
            }
        }
    }

    /* Grunts. */
    for (int i = 0; i < s_grunt_count; i++) {
        grunt_t *gr = &s_grunts[i];
        if (!gr->alive) {
            continue;
        }
        if (!gr->on_ground) {
            gr->vy += GRAV;
            if (gr->vy > MAX_FALL) {
                gr->vy = MAX_FALL;
            }
        }
        gr->x += gr->dir * (int)0x28;
        gr->y += gr->vy;
        const int gx = gr->x >> 6;
        const int gy = gr->y >> 6;
        if (box_collides(gx, gy, 8, 8)) {
            if (gr->vy > 0) {
                const int row = (gy + 8 - 1) / TILE;
                gr->y = (row * TILE - 8) << 6;
                gr->vy = 0;
                gr->on_ground = true;
            } else {
                gr->vy = 0;
                gr->on_ground = false;
                gr->dir = -gr->dir;
            }
        }
        /* Turn around at pits / edges / patrol distance. */
        const int col = gx / TILE;
        const int ahead = (gr->dir > 0) ? col + 1 : col - 1;
        if (ahead < 0 || ahead >= WORLD_COLS || s_col[ahead].ground <= 0 ||
            s_col[ahead].plat >= 0) {
            gr->travel = 30;
            gr->dir = -gr->dir;
        } else if (s_col[col].ground <= 0) {
            gr->travel = 30;
            gr->dir = -gr->dir;
        }
        gr->travel += (int)0x28;
        if (gr->travel > 50 + s_level * 24) {
            gr->travel = 30;
            gr->dir = -gr->dir;
        }
    }

    /* Hero vs grunts. */
    {
        const int hx = s_hero.px >> 6, hy = s_hero.py >> 6;
        const int hb = hy + P_H;
        for (int i = 0; i < s_grunt_count; i++) {
            grunt_t *gr = &s_grunts[i];
            if (!gr->alive) {
                continue;
            }
            const int gx = (gr->x >> 6) + 1, gy = gr->y >> 6;
            if (hx + P_W <= gx || hx >= gx + 7 || hb <= gy ||
                hy >= gy + 8) {
                continue;
            }
            if (s_hero.vy > 0 && hb <= gy + 5) {
                gr->alive = false;
                s_score += 25;
                s_hero.vy = -STOMP_BOUNCE;
                s_hero.on_ground = false;
            } else {
                player_dies();
                return;
            }
        }
    }

    /* Goal flag. */
    {
        const int flag_x = (WORLD_COLS - 3) * TILE - 2;
        if ((s_hero.px >> 6) + P_W >= flag_x) {
            s_score += 200u;
            s_state = RUN_WON;
            s_state_ticks = 0;
        }
    }
}

static void draw_tiles(void)
{
    const int c0 = s_cam / TILE, c1 = (s_cam - 1 + DISPLAY_WIDTH) / TILE;
    for (int cc = c0; cc <= c1; cc++) {
        if (cc < 0 || cc >= WORLD_COLS) {
            continue;
        }
        const int sx = cc * TILE - s_cam;
        const col_t *col = &s_col[cc];
        if (col->ground > 0) {
            const int top = (TILES_Y - col->ground) * TILE;
            display_fill_rect(sx, top, TILE, TILE, C_GRASS);
            display_fill_rect(sx, top + TILE, TILE, DISPLAY_HEIGHT - top - TILE,
                              C_DIRT);
        }
        if (col->plat >= 0) {
            display_fill_rect(sx, col->plat * TILE, TILE, TILE, C_GRASS);
            display_fill_rect(sx, col->plat * TILE + 6, TILE, 2, C_DIRT);
        }
    }
}

void runner_render(void)
{
    display_clear(DISPLAY_COLOR_BLACK);

    draw_tiles();

    /* Coins. */
    const int c0 = s_cam / TILE, c1 = (s_cam - 1 + DISPLAY_WIDTH) / TILE;
    for (int cc = c0; cc <= c1; cc++) {
        if (cc < 0 || cc >= WORLD_COLS || !s_col[cc].coin) {
            continue;
        }
        const int row = surface_row(cc) - 1;
        display_fill_rect(cc * TILE - s_cam + 2, row * TILE + 2, 4, 4,
                          DISPLAY_COLOR_YELLOW);
    }

    /* Grunts. */
    for (int i = 0; i < s_grunt_count; i++) {
        const grunt_t *gr = &s_grunts[i];
        if (!gr->alive) {
            continue;
        }
        const int gx = (gr->x >> 6) - s_cam;
        const int gy = gr->y >> 6;
        display_fill_rect(gx + 1, gy + 1, 6, 7, DISPLAY_COLOR_RED);
        display_fill_rect(gx + 2, gy + 3, 4, 2, DISPLAY_COLOR_WHITE);
    }

    /* Goal flag. */
    {
        const int fx = (WORLD_COLS - 3) * TILE - s_cam - 2;
        const int fbottom = (TILES_Y - GROUND) * TILE;
        display_fill_rect(fx + 7, fbottom - 4 * TILE, 1,
                          fbottom - (fbottom - 4 * TILE),
                          DISPLAY_COLOR_MAGENTA);
        display_fill_rect(fx + 8, fbottom - 4 * TILE, 6, 5,
                          DISPLAY_COLOR_YELLOW);
        display_fill_rect(fx + 5, fbottom - 2 * TILE, 7, 6,
                          DISPLAY_COLOR_MAGENTA);
        display_fill_rect(fx + 8, fbottom - TILE, 4, TILE,
                          DISPLAY_COLOR_MAGENTA);
    }

    /* Hero. */
    {
        const int hx = (s_hero.px >> 6) - s_cam;
        const int hy = s_hero.py >> 6;
        display_fill_rect(hx, hy, 8, 3, DISPLAY_COLOR_CYAN);
        display_fill_rect(hx, hy + 3, 8, 9, DISPLAY_COLOR_BLUE);
        display_fill_rect(s_hero.dir > 0 ? hx + 6 : hx, hy + 4, 2, 2,
                          DISPLAY_COLOR_YELLOW);
    }

    char buf[20];
    int n = snprintf(buf, sizeof(buf), "SCORE %04u", (unsigned)s_score);
    if (n > 0) {
        display_draw_text(2, 2, buf, DISPLAY_COLOR_WHITE, DISPLAY_COLOR_BLACK);
    }
    for (int i = 0; i < s_lives; i++) {
        display_fill_rect(DISPLAY_WIDTH - 5 - i * 4, 5, 3, 3,
                          DISPLAY_COLOR_CYAN);
    }
    n = snprintf(buf, sizeof(buf), "LV%02u", (unsigned)s_level);
    if (n > 0) {
        display_draw_text(DISPLAY_WIDTH - 34, 2, buf, DISPLAY_COLOR_GRAY,
                          DISPLAY_COLOR_BLACK);
    }

    if (s_state == RUN_WON) {
        display_fill_rect(30, 60, 100, 24, DISPLAY_COLOR_DKGRAY);
        display_draw_text(38, 64, "LEVEL", DISPLAY_COLOR_GREEN,
                          DISPLAY_COLOR_DKGRAY);
        display_draw_text(44, 73, "CLEAR!", DISPLAY_COLOR_WHITE,
                          DISPLAY_COLOR_DKGRAY);
    }
    if (s_state == RUN_DEAD) {
        display_fill_rect(38, 60, 84, 24, DISPLAY_COLOR_DKGRAY);
        display_draw_text(46, 64, "GAME OVER", DISPLAY_COLOR_RED,
                          DISPLAY_COLOR_DKGRAY);
        display_draw_text(46, 73, "PRESS A", DISPLAY_COLOR_WHITE,
                          DISPLAY_COLOR_DKGRAY);
    }
}