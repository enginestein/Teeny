/*
 * pacmate.c -- PACMATE, a little maze-muncher for Teeny.
 *
 * A PAC*-style chase around a symmetric 20x18 corridor maze. Munch dots for
 * points, grab the big power pellets to flip the resident ghost blue and eat
 * it for a bonus, and clear every dot to move on to the next (speedier) round.
 * Lives, fright timers and the HUD are all inline. Grid movement: entities
 * travel centre-to-centre at one pixel per tick and only pick a new direction
 * at cell centres.
 */

#include "teeny/pacmate.h"

#include "teeny/display.h"
#include "teeny/input.h"

#include "pico/stdlib.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TILE     8
#define MAZE_W   (DISPLAY_WIDTH / TILE)    /* 20 */
#define MAZE_H   (DISPLAY_HEIGHT / TILE)   /* 18 */
#define START_DOT_ROW 1
#define START_DOT_COL 1
#define HOME_COL 9
#define HOME_ROW 9
#define MAX_DOTS 208
#define MAX_LIVES 3
#define FRIGHT_BASE 240
#define INVULN 120

static const char s_maze[MAZE_H][MAZE_W + 1] = {
    "####################",
    "#......#....#......#",
    "#.####.#....#.####.#",
    "#.####.#....#.####.#",
    "#.o....#....#....o.#",
    "#.##...##..##...##.#",
    "#.##...##..##...##.#",
    "#..................#",
    "#.......    .......#",
    "#.......    .......#",
    "#..................#",
    "#.##...##..##...##.#",
    "#.##...##..##...##.#",
    "#.o....#....#....o.#",
    "#.####.#....#.####.#",
    "#.####.#....#.####.#",
    "#......#....#......#",
    "####################",
};

enum {
    DIR_UP=0, DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_NONE,
};

enum { GH_CHASE = 0, GH_FRIGHT = 1, GH_DEAD = 2 };

static const int DI_DX[5] = { 0, 0, -1, 1, 0 };
static const int DI_DY[5] = { -1, 1, 0, 0, 0 };

static char      s_map[MAZE_H][MAZE_W];
static int       s_px, s_py;      /* player top-left, pixels, 6x6 */
static int       s_pdir;
static int       s_want;          /* desired dir from the D-pad */
static int       s_lives;
static int       s_fright;
static int       s_invuln;
static int       s_gx, s_gy;      /* ghost top-left, pixels, 8x8 */
static int       s_gdir;
static int       s_gmode;
static unsigned  s_score;
static int       s_level;
static int       s_dots;
static int       s_state;         /* 0 play / 1 win / 2 over */
static uint16_t  s_ticks;
static uint32_t  s_rand;

static uint32_t prng_next(void)
{
    uint32_t x = s_rand;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s_rand = x != 0 ? x : 0xA5A5A5A5u;
    return s_rand;
}

static void load_map(void)
{
    s_dots = 0;
    for (int r = 0; r < MAZE_H; r++) {
        for (int c = 0; c < MAZE_W; c++) {
            s_map[r][c] = s_maze[r][c];
            if (s_map[r][c] == '.') {
                s_dots++;
            }
        }
    }
}

static void reset_positions(void)
{
    s_px = (START_DOT_COL + 0) * TILE + 1;
    s_py = (START_DOT_ROW + 0) * TILE + 1;
    s_pdir = DIR_NONE;
    s_want = DIR_NONE;
    s_gx = HOME_COL * TILE;
    s_gy = HOME_ROW * TILE;
    s_gdir = DIR_UP;
    s_gmode = GH_CHASE;
    s_fright = 0;
}

void pacmate_init(void)
{
    s_rand = 0xBEEFu ^ (uint32_t)time_us_32();
    s_score = 0;
    s_lives = MAX_LIVES;
    s_level = 1;
    s_invuln = 0;
    s_state = 0;
    s_ticks = 0;
    load_map();
    reset_positions();
}

unsigned pacmate_score(void)
{
    return s_score;
}

static bool open_cell(int c, int r)
{
    if (c < 0 || c >= MAZE_W || r < 0 || r >= MAZE_H) {
        return false;
    }
    return s_map[r][c] != '#';
}

static int cell_of(int n)
{
    return n >> 3;
}

static int dist_cells(int cy, int cx, int ty, int tx)
{
    int d = cy - ty;
    if (d < 0) {
        d = -d;
    }
    int e = cx - tx;
    if (e < 0) {
        e = -e;
    }
    return d + e;
}

static void eat_cell(void)
{
    const int r = cell_of(s_py + 3);
    const int c = cell_of(s_px + 3);
    if (s_map[r][c] == '.') {
        s_map[r][c] = ' ';
        s_dots--;
        s_score += 10;
    } else if (s_map[r][c] == 'o') {
        s_map[r][c] = ' ';
        s_score += 50;
        s_fright = FRIGHT_BASE + s_level * 30;
        s_gmode = GH_FRIGHT;
    }
}

/* Greedy best-first direction toward a target cell; prefers straight runs
 * over the reverse, and never picks a blocked cell. Falls back to whatever
 * open direction is available so the ghost never spins forever. */
static int pick_dir(int gc, int gr, int tgc, int tgr)
{
    int bd = 1 << 20;
    int best = DIR_NONE;
    for (int d = 0; d < 4; d++) {
        const int nc = gc + DI_DX[d], nr = gr + DI_DY[d];
        if (!open_cell(nc, nr)) {
            continue;
        }
        if ((s_gdir == DIR_UP && d == DIR_DOWN) ||
            (s_gdir == DIR_DOWN && d == DIR_UP) ||
            (s_gdir == DIR_LEFT && d == DIR_RIGHT) ||
            (s_gdir == DIR_RIGHT && d == DIR_LEFT)) {
            continue;
        }
        const int dg = dist_cells(nr, nc, tgr, tgc);
        if (dg < bd) {
            bd = dg;
            best = d;
        }
    }
    if (best != DIR_NONE) {
        return best;
    }
    for (int d = 0; d < 4; d++) {
        if (open_cell(gc + DI_DX[d], gr + DI_DY[d])) {
            return d;
        }
    }
    return DIR_NONE;
}

static void step_entity(int *x, int *y, int dir)
{
    *x += DI_DX[dir];
    *y += DI_DY[dir];
}

void pacmate_update(void)
{
    if (s_state == 2) {
        if (input_edge_pressed(BTN_A) || input_edge_pressed(BTN_START)) {
            pacmate_init();
        }
        return;
    }
    if (s_state == 1) {
        s_ticks++;
        if (s_ticks > 90) {
            s_level++;
            s_score += 50u * s_level;
            s_ticks = 0;
            load_map();
            reset_positions();
            s_state = 0;
        }
        return;
    }

    if (s_fright > 0) {
        s_fright--;
        if (s_fright == 0 && s_gmode == GH_FRIGHT) {
            s_gmode = GH_CHASE;
        }
    }
    if (s_invuln > 0) {
        s_invuln--;
    }

    /* Player direction input. */
    s_want = DIR_NONE;
    if (input_is_pressed(BTN_UP)) {
        s_want = DIR_UP;
    } else if (input_is_pressed(BTN_DOWN)) {
        s_want = DIR_DOWN;
    } else if (input_is_pressed(BTN_LEFT)) {
        s_want = DIR_LEFT;
    } else if (input_is_pressed(BTN_RIGHT)) {
        s_want = DIR_RIGHT;
    }

    /* Player movement (centre-to-centre stepping). */
    {
        const int pc = cell_of(s_px + 3), pr = cell_of(s_py + 3);
        /* Centre pixel has reached a cell centre: time to turn / eat. */
        const bool at_centre = ((s_px + 3) & 7) == 4 && ((s_py + 3) & 7) == 4;
        if (at_centre) {
            eat_cell();
            if (s_want != DIR_NONE &&
                open_cell(pc + DI_DX[s_want], pr + DI_DY[s_want])) {
                s_pdir = s_want;
            }
            if (!open_cell(pc + DI_DX[s_pdir], pr + DI_DY[s_pdir])) {
                s_pdir = DIR_NONE;
            }
        }
        if (s_pdir != DIR_NONE) {
            s_px += DI_DX[s_pdir];
            s_py += DI_DY[s_pdir];
        }
    }

    /* Ghost update (chase / fright / returning home). */
    {
        const int gc = cell_of(s_gx + 4), gr = cell_of(s_gy + 4);
        /* Ghost box aligned to a tile => centre pixel is at a cell centre. */
        const bool at_centre = ((s_gx + 4) & 7) == 4 && ((s_gy + 4) & 7) == 4;
        if (at_centre) {
            int target_c = gc, target_r = gr;
            if (s_gmode == GH_FRIGHT) {
                int arrow = -1;
                for (int tries = 0; tries < 8 && arrow < 0; tries++) {
                    const int r2 = (int)(prng_next() % 4);
                    if (open_cell(gc + DI_DX[r2], gr + DI_DY[r2])) {
                        arrow = r2;
                    }
                }
                s_gdir = arrow >= 0 ? arrow : s_gdir;
            } else if (s_gmode == GH_DEAD) {
                target_c = HOME_COL;
                target_r = HOME_ROW;
                s_gdir = pick_dir(gc, gr, target_c, target_r);
            } else {
                target_c = cell_of(s_px + 3);
                target_r = cell_of(s_py + 3);
                s_gdir = pick_dir(gc, gr, target_c, target_r);
            }
            if (s_gdir == DIR_NONE ||
                !open_cell(gc + DI_DX[s_gdir], gr + DI_DY[s_gdir])) {
                s_gdir = DIR_NONE;
            }
        }
        if (s_gdir != DIR_NONE) {
            step_entity(&s_gx, &s_gy, s_gdir);
        }

        /* Returning home: back to life on arrival. */
        if (s_gmode == GH_DEAD &&
            cell_of(s_gx + 4) == HOME_COL && cell_of(s_gy + 4) == HOME_ROW) {
            s_gmode = GH_CHASE;
        }
    }

    /* Ghost vs player. */
    if (s_invuln == 0) {
        const int pc = cell_of(s_px + 3), pr = cell_of(s_py + 3);
        const int gc = cell_of(s_gx + 4), gr = cell_of(s_gy + 4);
        if (s_gmode != GH_DEAD && pc == gc && pr == gr) {
            if (s_gmode == GH_FRIGHT) {
                s_gmode = GH_DEAD;
                s_score += 200;
            } else {
                s_lives--;
                if (s_lives <= 0) {
                    s_state = 2;
                    return;
                }
                s_invuln = INVULN;
                reset_positions();
            }
        }
    }

    /* Round cleared? */
    if (s_dots <= 0) {
        s_state = 1;
        s_ticks = 0;
    }
}

void pacmate_render(void)
{
    display_clear(DISPLAY_COLOR_BLACK);

    for (int r = 0; r < MAZE_H; r++) {
        for (int c = 0; c < MAZE_W; c++) {
            const char ch = s_map[r][c];
            const int x = c * TILE, y = r * TILE;
            if (ch == '#') {
                display_fill_rect(x, y, TILE, TILE, DISPLAY_COLOR_BLUE);
            } else if (ch == '.') {
                display_set_pixel(x + 3, y + 3, DISPLAY_COLOR_WHITE);
                display_set_pixel(x + 4, y + 3, DISPLAY_COLOR_WHITE);
                display_set_pixel(x + 3, y + 4, DISPLAY_COLOR_WHITE);
                display_set_pixel(x + 4, y + 4, DISPLAY_COLOR_WHITE);
            } else if (ch == 'o') {
                display_fill_rect(x + 1, y + 1, 6, 6, DISPLAY_COLOR_MAGENTA);
            } else {
                /* the little haunt: dim floor so the pocket reads as a room */
                if ((r == 8 || r == 9) && c >= 8 && c <= 11) {
                    display_set_pixel(x + 3, y + 3, DISPLAY_COLOR_DKGRAY);
                    display_set_pixel(x + 4, y + 4, DISPLAY_COLOR_DKGRAY);
                }
            }
        }
    }

    /* Ghost. */
    if (s_gmode != GH_DEAD) {
        const bool blink = (s_gmode == GH_FRIGHT) &&
                           ((s_fright / 12) % 2 == 0);
        const display_color_t gc = blink ? DISPLAY_COLOR_WHITE
                                         : DISPLAY_COLOR_CYAN;
        if (s_gmode == GH_FRIGHT) {
            display_fill_rect(s_gx, s_gy, TILE, TILE, gc);
        } else {
            display_fill_rect(s_gx + 1, s_gy + 1, 6, 7,
                              DISPLAY_COLOR(255, 130, 0));
            display_set_pixel(s_gx + 2, s_gy + 3, DISPLAY_COLOR_WHITE);
            display_set_pixel(s_gx + 5, s_gy + 3, DISPLAY_COLOR_WHITE);
        }
    }

    /* Player (flash while invulnerable after a hit). */
    if (s_invuln == 0 || (s_invuln / 6) % 2 == 0) {
        display_fill_rect(s_px, s_py, 6, 6, DISPLAY_COLOR_YELLOW);
        display_fill_rect(s_px + 1, s_py + 1, 2, 1, DISPLAY_COLOR_BLACK);
    }

    /* HUD. */
    char buf[16];
    int n = snprintf(buf, sizeof(buf), "SC %05u", (unsigned)s_score);
    if (n > 0) {
        static const display_color_t hud_bg = DISPLAY_COLOR_BLUE;
        display_draw_text(1, 1, buf, DISPLAY_COLOR_WHITE, hud_bg);
    }
    for (int i = 0; i < s_lives; i++) {
        display_fill_rect(DISPLAY_WIDTH - 6 - i * 4, 2, 3, 3,
                          DISPLAY_COLOR_YELLOW);
    }

    if (s_state == 1) {
        display_fill_rect(38, 60, 84, 24, DISPLAY_COLOR_DKGRAY);
        display_draw_text(46, 64, "CLEAR!", DISPLAY_COLOR_GREEN,
                          DISPLAY_COLOR_DKGRAY);
        display_draw_text(46, 73, "ROUND 2", DISPLAY_COLOR_WHITE,
                          DISPLAY_COLOR_DKGRAY);
    }
    if (s_state == 2) {
        display_fill_rect(38, 60, 84, 24, DISPLAY_COLOR_DKGRAY);
        display_draw_text(46, 64, "GAME OVER", DISPLAY_COLOR_RED,
                          DISPLAY_COLOR_DKGRAY);
        display_draw_text(46, 73, "PRESS A", DISPLAY_COLOR_WHITE,
                          DISPLAY_COLOR_DKGRAY);
    }
}