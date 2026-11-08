/*
 * snake.h -- Snake for Teeny (Phase 5 of the roadmap).
 *
 * A self-contained game loop step: the kernel drives a fixed-timestep tick
 * (~60 Hz) by calling snake_update() each tick, then snake_render() when a
 * frame should be presented. The game owns no hardware knowledge -- it only
 * reads the abstract console buttons (input.h) and draws via display.h, so
 * the same binary logic runs against the PC viewer today and a SPI TFT +
 * GPIO buttons on final hardware tomorrow.
 */

#ifndef TEENY_SNAKE_H
#define TEENY_SNAKE_H

void snake_init(void);
void snake_update(void);  /* one fixed-60Hz sim tick */
void snake_render(void);  /* draw the current state into the back buffer */
unsigned snake_score(void);

#endif /* TEENY_SNAKE_H */