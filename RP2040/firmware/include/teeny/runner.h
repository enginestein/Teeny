/*
 * runner.h -- RUNNER, a sidescrolling platformer for Teeny (catalog).
 *
 * Pure client of the display.h/input.h abstractions: kernel calls
 * runner_update() at ~60 Hz and runner_render() whenever a frame is due.
 */

#ifndef TEENY_RUNNER_H
#define TEENY_RUNNER_H

void runner_init(void);
void runner_update(void);  /* one fixed-60Hz sim tick */
void runner_render(void);  /* draw the current state into the back buffer */
unsigned runner_score(void);

#endif /* TEENY_RUNNER_H */