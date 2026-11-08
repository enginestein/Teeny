/*
 * launcher.c -- title menu for Teeny.
 *
 * A simple vertical list: the selected title is drawn yellow, the others
 * dimmed gray. 'TEENY' sits in a small logo strip on top. The design is
 * deliberately plain -- 5x7 text only -- so it stays readable at 4x on the
 * PC viewer and tiny on a real panel.
 */

#include "teeny/launcher.h"

#include "teeny/display.h"
#include "teeny/input.h"

#define MENU_TOP     42
#define LINE_GAP     14
#define MAX_VISIBLE  6

static int s_selection;

enum {
    MENU_COLOR_TITLE = DISPLAY_COLOR_YELLOW,
    MENU_COLOR_ITEM  = DISPLAY_COLOR_WHITE,
    MENU_COLOR_DIM   = DISPLAY_COLOR_GRAY,
};

void launcher_init(void)
{
    s_selection = 0;
}

void launcher_update(int game_count)
{
    if (input_edge_pressed(BTN_DOWN) && s_selection + 1 < game_count) {
        s_selection++;
    }
    if (input_edge_pressed(BTN_UP) && s_selection > 0) {
        s_selection--;
    }
}

int launcher_selection(void)
{
    return s_selection;
}

void launcher_render(const char *const *titles, int count)
{
    display_clear(DISPLAY_COLOR_BLACK);

    display_draw_text(58, 8, "TEENY", DISPLAY_COLOR_GREEN, DISPLAY_COLOR_BLACK);
    display_fill_rect(38, 18, 84, 1, DISPLAY_COLOR_DKGRAY);

    const int first = s_selection < MAX_VISIBLE ? 0 : s_selection - MAX_VISIBLE + 1;
    const int last = first + MAX_VISIBLE < count ? first + MAX_VISIBLE : count;

    for (int i = first; i < last; i++) {
        const int y = MENU_TOP + (i - first) * LINE_GAP;
        if (i == s_selection) {
            display_fill_rect(6, y - 1, 148, 9, DISPLAY_COLOR_DKGRAY);
            display_draw_text(14, y, titles[i], MENU_COLOR_TITLE, DISPLAY_COLOR_DKGRAY);
        } else {
            display_draw_text(14, y, titles[i], MENU_COLOR_DIM, DISPLAY_COLOR_BLACK);
        }
    }

    display_draw_text(8, 132, "A:START", MENU_COLOR_DIM, DISPLAY_COLOR_BLACK);
    display_draw_text(66, 132, "SEL:MENU", MENU_COLOR_DIM, DISPLAY_COLOR_BLACK);
}