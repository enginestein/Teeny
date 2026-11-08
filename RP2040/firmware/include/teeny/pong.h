/*
 * pong.h -- Pong for Teeny (launcher catalog).
 *
 * Self-contained fixed-timestep game: the kernel calls pong_update() at
 * ~60 Hz and pong_render() whenever a frame is due. Pure client of the
 * display.h/input.h abstractions.
 */

#ifndef TEENY_PONG_H
#define TEENY_PONG_H

void pong_init(void);
void pong_update(void);  /* one fixed-60Hz sim tick */
void pong_render(void);  /* draw the current state into the back buffer */
unsigned pong_score(void);

#endif /* TEENY_PONG_H */