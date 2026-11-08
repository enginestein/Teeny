/*
 * phase1_pattern.h -- Phase-1 test pattern.
 *
 * Renders a moving, recognisable image into the Display framebuffer so the
 * USB framebuffer link can be verified by eye: colour bars, a grey ramp, a
 * checkerboard, a bouncing ball and a sweeping scanline. All motion is pure
 * integer math driven by a millisecond timestamp.
 */

#ifndef TEENY_PHASE1_PATTERN_H
#define TEENY_PHASE1_PATTERN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Paint the test pattern into the current display back-buffer. */
void phase1_pattern_render(uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif /* TEENY_PHASE1_PATTERN_H */