#ifndef TEENY_BOOT_H
#define TEENY_BOOT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Print the boot banner and basic system info as PKT_BOOT frames.
 * Does not touch peripherals. */
void boot_print_banner(void);

#ifdef __cplusplus
}
#endif

#endif /* TEENY_BOOT_H */