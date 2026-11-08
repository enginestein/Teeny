/*
 * launcher.h -- Game-Boy-style title menu for Teeny.
 *
 * Purely presentational + one input-driven selection. The kernel decides what
 * happens on confirm (START/A) and owns the current game instance; this
 * module only draws the menu and tracks which title is highlighted.
 */

#ifndef TEENY_LAUNCHER_H
#define TEENY_LAUNCHER_H

void launcher_init(void);
/* Poll the D-pad edges and move the highlight (clamped to count games). */
void launcher_update(int game_count);
int  launcher_selection(void);
void launcher_render(const char *const *titles, int count);

#endif /* TEENY_LAUNCHER_H */