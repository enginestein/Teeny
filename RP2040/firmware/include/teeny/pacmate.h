/*
 * pacmate.h -- PACMATE, a maze muncher for Teeny (catalog).
 *
 * Pure client of the display.h/input.h abstractions: kernel calls
 * pacmate_update() at ~60 Hz and pacmate_render() whenever a frame is due.
 */

#ifndef TEENY_PACMATE_H
#define TEENY_PACMATE_H

void pacmate_init(void);
void pacmate_update(void);  /* one fixed-60Hz sim tick */
void pacmate_render(void);  /* draw the current state into the back buffer */
unsigned pacmate_score(void);

#endif /* TEENY_PACMATE_H */