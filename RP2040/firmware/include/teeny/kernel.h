/*
 * kernel.h -- Teeny main kernel.
 *
 * The kernel owns boot order, the main loop, and the diagnostics heartbeat.
 * Everything else (USB link, SD, storage) is a module it starts and polls.
 */

#ifndef TEENY_KERNEL_H
#define TEENY_KERNEL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Initialise everything and run the console main loop forever. */
void kernel_run(void);

#ifdef __cplusplus
}
#endif

#endif /* TEENY_KERNEL_H */