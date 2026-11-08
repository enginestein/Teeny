/*
 * invaders.h -- Space Invaders-style shooter for Teeny.
 */

#ifndef TEENY_INVADERS_H
#define TEENY_INVADERS_H

void invaders_init(void);
void invaders_update(void);  /* one fixed 60 Hz sim tick */
void invaders_render(void);
unsigned invaders_score(void);

#endif /* TEENY_INVADERS_H */