/*
 * game.h -- The Teeny game interface (Phase 6: launcher).
 *
 * Every game is a small struct of functions plus a title. The kernel runs a
 * fixed 60 Hz tick and calls update() once per tick; render() draws the
 * current state into the display back buffer whenever a frame is due. Games
 * are pure clients of the platform abstractions (display.h, input.h) -- no
 * USB, no timers, no hardware knowledge -- so the exact same binaries run on
 * the PC-viewer dev rig and on a future console with a SPI TFT and a GPIO
 * button matrix.
 *
 * Convention: the KERNEL owns the menu/launcher button "SELECT" (exit game),
 * and the START button launches/confirms in the menu. Games use the D-pad,
 * A and B freely; a game that wants a confirm/restart button uses A.
 */

#ifndef TEENY_GAME_H
#define TEENY_GAME_H

typedef struct game_app {
    const char *title;              /* shown in the launcher (<= 14 chars) */
    void (*init)(void);             /* reset to a fresh run/state           */
    void (*update)(void);           /* one fixed 60 Hz simulation tick      */
    void (*render)(void);           /* draw current state, back buffer      */
    unsigned (*score)(void);        /* current score (menu HUD)             */
} game_app_t;

#endif /* TEENY_GAME_H */