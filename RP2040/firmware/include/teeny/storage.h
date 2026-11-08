/*
 * storage.h -- Persistent storage (microSD card behind a FAT filesystem).
 *
 * The SD card is the console's persistent store and is never assumed to be
 * present. Every call is tolerant: if the card is missing or unreadable the
 * storage layer reports an error and the console keeps running.
 *
 * Layout (created on first successful mount):
 *
 *     /games    game binaries / assets (phases 2+)
 *     /saves    game save data
 *     /config   console configuration
 *     /system   internal diagnostics
 */

#ifndef TEENY_STORAGE_H
#define TEENY_STORAGE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STORAGE_MISSING = -2, /* no card detected on the SPI bus */
    STORAGE_ERROR   = -1, /* card present but init / mount failed */
    STORAGE_OK      = 0,
} storage_result_t;

/* Mount the FAT volume and create the standard directory layout.
 * Safe to call repeatedly; re-mounts if the card was removed. */
storage_result_t storage_init(void);

/* Phase-0 diagnostic: create /system/diag.txt, write a line, read it back
 * and verify. Returns STORAGE_OK on a full round-trip. */
storage_result_t storage_diag_roundtrip(void);

/* True once storage_init() has successfully mounted a volume. */
bool storage_is_mounted(void);

#ifdef __cplusplus
}
#endif

#endif /* TEENY_STORAGE_H */