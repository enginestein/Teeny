/*
 * snake.c -- Snake implementation for Teeny.
 *
 * The playfield is a 20x16 grid of 8px cells (160x128 px) with a 16px HUD
 * strip on top for the score, so a full 160x144 frame. Snake body, apple and
 * walls are fixed-size cell fills -- no heap, no sprites, no physics engine;
 * this is the deliberate Phase-5 scope check for the graphics layer.
 */

#include "teeny/snake.h"

#include "teeny/display.h"
#include "teeny/input.h"

#include "pico/stdlib.h"

#include <stdint.h>

#define CELL_PX  8
#define GRID_W   (DISPLAY_WIDTH / CELL_PX)   /* 20 */
#define GRID_H   ((DISPLAY_HEIGHT - HUD_PX) / CELL_PX) /* 16 */
#define HUD_PX   16
#define MAX_LEN  (GRID_W * GRID_H)

typedef struct {
    uint8_t x, y;
} snake_pt_t;

typedef enum {
    DIR_UP = 0,
    DIR_DOWN = 1,
    DIR_LEFT = 2,
    DIR_RIGHT = 3
} snake_dir_t;

static snake_pt_t s_body[MAX_LEN];
static uint16_t  s_len;
static snake_dir_t s_dir;
static snake_dir_t s_next_dir;
static snake_pt_t s_apple;
static uint16_t  s_score;
static bool      s_alive;
static uint8_t   s_tick_acc;      /* counts 60Hz ticks between moves */
static uint8_t   s_move_ticks;    /* ticks per snake step (60/move_ticks per s) */
static uint32_t  s_rand;

/* xmove / ymove indexed by snake_dir_t */
static const int8_t DX[4] = { 0, 0, -1, 1 };
static const int8_t DY[4] = { -1, 1, 0, 0 };

static bool dir_is_opposite(snake_dir_t a, snake_dir_t b)
{
    return (a == DIR_UP && b == DIR_DOWN) || (a == DIR_DOWN && b == DIR_UP) ||
           (a == DIR_LEFT && b == DIR_RIGHT) || (a == DIR_RIGHT && b == DIR_LEFT);
}

static uint32_t prng_next(void)
{
    /* xorshift32: cheap, deterministic, no libc rand() dependency. */
    uint32_t x = s_rand;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s_rand = x != 0 ? x : 0xA5A5A5A5u;
    return s_rand;
}

static bool cell_occupied(uint8_t cx, uint8_t cy, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        if (s_body[i].x == cx && s_body[i].y == cy) {
            return true;
        }
    }
    return false;
}

static void place_apple(void)
{
    for (uint32_t attempt = 0; attempt < MAX_LEN * 2u; attempt++) {
        const uint8_t cx = (uint8_t)(prng_next() % GRID_W);
        const uint8_t cy = (uint8_t)(prng_next() % GRID_H);
        if (!cell_occupied(cx, cy, s_len)) {
            s_apple.x = cx;
            s_apple.y = cy;
            return;
        }
    }
}

void snake_init(void)
{
    s_len = 0;
    s_score = 0;
    s_alive = true;
    s_dir = DIR_RIGHT;
    s_next_dir = DIR_RIGHT;
    s_move_ticks = 10;  /* 6 cells/second at start */
    s_tick_acc = 0;
    s_rand = 0x1234567u ^ (uint32_t)time_us_32();

    /* Start centered: three cells, head in the middle of the field. */
    const uint8_t cy = (uint8_t)(GRID_H / 2);
    const uint8_t cx_head = (uint8_t)(GRID_W / 2);
    for (int i = 0; i < 3; i++) {
        s_body[i].x = (uint8_t)(cx_head - 1 - i);
        s_body[i].y = cy;
    }
    s_len = 3;
    place_apple();
}

unsigned snake_score(void)
{
    return s_score;
}

static void snake_die(void)
{
    s_alive = false;
}

static void snake_step(void)
{
    /* Pick the direction the player queued, unless it would reverse us. */
    if (!dir_is_opposite(s_next_dir, s_dir)) {
        s_dir = s_next_dir;
    }

    snake_pt_t head = {
        .x = (uint8_t)(s_body[0].x + DX[s_dir]),
        .y = (uint8_t)(s_body[0].y + DY[s_dir]),
    };

    /* Wall collision: the border of the grid is deadly. */
    if (head.x >= GRID_W || head.y >= GRID_H) {
        snake_die();
        return;
    }

    const bool will_eat = (head.x == s_apple.x && head.y == s_apple.y);

    /* Self collision. When eating, the tail does not vacate this tick, so
     * the whole body counts; otherwise the tail cell is safe to enter. */
    const uint16_t check_len = will_eat ? s_len : (uint16_t)(s_len - 1);
    if (cell_occupied(head.x, head.y, check_len)) {
        snake_die();
        return;
    }

    /* Shift: insert new head, drop tail unless growing. */
    if (will_eat && s_len < MAX_LEN) {
        s_len++;
    }
    for (int i = s_len - 1; i > 0; i--) {
        s_body[i] = s_body[i - 1];
    }
    s_body[0] = head;

    if (will_eat) {
        s_score++;
        place_apple();
        /* Gentle ramp: every 6 apples, one tick faster (min 6). */
        if ((s_score % 6) == 0 && s_move_ticks > 6) {
            s_move_ticks--;
        }
    }
}

void snake_update(void)
{
    /* Buffer direction every tick so turns are never missed between moves. */
    if (input_is_pressed(BTN_UP)) {
        s_next_dir = DIR_UP;
    } else if (input_is_pressed(BTN_DOWN)) {
        s_next_dir = DIR_DOWN;
    } else if (input_is_pressed(BTN_LEFT)) {
        s_next_dir = DIR_LEFT;
    } else if (input_is_pressed(BTN_RIGHT)) {
        s_next_dir = DIR_RIGHT;
    }

    if (!s_alive) {
        /* A or START restarts. */
        if (input_is_pressed(BTN_A) || input_is_pressed(BTN_START)) {
            snake_init();
        }
        return;
    }

    s_tick_acc++;
    if (s_tick_acc >= s_move_ticks) {
        s_tick_acc = 0;
        snake_step();
    }
}

static void draw_score(void)
{
    /* "SCORE 000" in the 16px HUD strip. */
    char hud[10] = "SCORE 000";
    hud[6] = (char)('0' + (s_score / 100) % 10);
    hud[7] = (char)('0' + (s_score / 10) % 10);
    hud[8] = (char)('0' + (s_score % 10));
    display_draw_text(4, 4, hud, DISPLAY_COLOR_WHITE, DISPLAY_COLOR_BLACK);
}

static void draw_walls(void)
{
    const int x0 = 0;
    const int x1 = DISPLAY_WIDTH - 1;
    const int y0 = HUD_PX;
    const int y1 = DISPLAY_HEIGHT - 1;
    display_fill_rect(x0, y0, DISPLAY_WIDTH, 1, DISPLAY_COLOR_GREEN);
    display_fill_rect(x0, y1, DISPLAY_WIDTH, 1, DISPLAY_COLOR_GREEN);
    display_fill_rect(x0, y0, 1, y1 - y0 + 1, DISPLAY_COLOR_GREEN);
    display_fill_rect(x1, y0, 1, y1 - y0 + 1, DISPLAY_COLOR_GREEN);
}

void snake_render(void)
{
    display_clear(DISPLAY_COLOR_BLACK);
    draw_score();
    draw_walls();

    const int oy = HUD_PX; /* field origin y in pixels */
    display_fill_rect(s_apple.x * CELL_PX, oy + s_apple.y * CELL_PX,
                      CELL_PX, CELL_PX, DISPLAY_COLOR_RED);

    if (s_alive) {
        for (uint16_t i = 0; i < s_len; i++) {
            const snake_pt_t p = s_body[i];
            const display_color_t c = (i == 0) ? DISPLAY_COLOR_YELLOW
                                               : DISPLAY_COLOR_GREEN;
            display_fill_rect(p.x * CELL_PX, oy + p.y * CELL_PX,
                              CELL_PX, CELL_PX, c);
        }
    }

    if (!s_alive) {
        /* Centered game-over plate over the field. */
        display_fill_rect(36, 56, 88, 32, DISPLAY_COLOR_BLACK);
        display_draw_text(46, 64, "GAME OVER", DISPLAY_COLOR_WHITE,
                          DISPLAY_COLOR_BLACK);
        display_draw_text(53, 76, "PRESS A", DISPLAY_COLOR_YELLOW,
                          DISPLAY_COLOR_BLACK);
    }
}