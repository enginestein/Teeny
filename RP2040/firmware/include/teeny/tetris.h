/*
 * tetris.h -- Tetris for Teeny (launcher catalog).
 *
 * Self-contained fixed-timestep game: the kernel calls tetris_update() at
 * ~60 Hz and tetris_render() whenever a frame is due. Pure client of the
 * display.h/input.h abstractions.
 */

#ifndef TEENY_TETRIS_H
#define TEENY_TETRIS_H

void tetris_init(void);
void tetris_update(void);  /* one fixed-60Hz sim tick */
void tetris_render(void);  /* draw the current state into the back buffer */
unsigned tetris_score(void);

#endif /* TEENY_TETRIS_H */