/*
 * flappy.h -- FLAPPY, a bird pipe-runner for Teeny (catalog).
 *
 * Pure client of the display.h/input.h abstractions: kernel calls
 * flappy_update() at ~60 Hz and flappy_render() whenever a frame is due.
 */

#ifndef TEENY_FLAPPY_H
#define TEENY_FLAPPY_H

void flappy_init(void);
void flappy_update(void);  /* one fixed-60Hz sim tick */
void flappy_render(void);  /* draw the current state into the back buffer */
unsigned flappy_score(void);

#endif /* TEENY_FLAPPY_H */