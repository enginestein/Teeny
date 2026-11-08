/*
 * breakout.h -- Breakout for Teeny (brick-busting paddle game).
 */

#ifndef TEENY_BREAKOUT_H
#define TEENY_BREAKOUT_H

void breakout_init(void);
void breakout_update(void);  /* one fixed 60 Hz sim tick */
void breakout_render(void);
unsigned breakout_score(void);

#endif /* TEENY_BREAKOUT_H */