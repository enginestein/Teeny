/*
 * storage.c -- microSD FAT storage for Teeny.
 *
 * FatFs (vendored under system/fatfs) is the filesystem; the SD SPI driver
 * (hardware/sd) is the disk interface. This module owns the mount lifecycle
 * and the standard directory layout, and provides the Phase-0 round-trip
 * diagnostic.
 *
 * Failure policy: the card is optional. If it is absent or unreadable,
 * storage_init() returns an error and nothing else in the console depends on
 * it. Games/launcher phases must handle STORAGE_MISSING gracefully.
 */

#include "teeny/storage.h"
#include "teeny/console.h"
#include "teeny/sd_spi.h"

#include "ff.h"
#include <stdio.h>
#include <string.h>

static FATFS  s_fatfs;
static bool   s_mounted;

/* FatFs timestamp. The RP2040 RTC is not yet wired into the filesystem;
 * a fixed build date is used so files carry a deterministic timestamp. */
DWORD get_fattime(void)
{
    return ((DWORD)(2026 - 1980) << 25) | /* year 2026 */
           ((DWORD)4 << 21) |             /* month 4            */
           ((DWORD)1 << 16) |             /* day   1            */
           ((DWORD)12 << 11) |            /* hour  12           */
           0u;
}

static void make_dir_if_missing(const char *path)
{
    FRESULT fr = f_mkdir(path);
    if (fr != FR_OK && fr != FR_EXIST) {
        console_log("storage: mkdir %s failed: %d", path, fr);
    }
}

storage_result_t storage_init(void)
{
    if (s_mounted) {
        return STORAGE_OK;
    }

    const FRESULT fr = f_mount(&s_fatfs, "", 1);
    if (fr != FR_OK) {
        const sd_info_t *sd = sd_info();
        console_log("storage: mount failed (%d)", fr);
        s_mounted = false;
        return sd->present ? STORAGE_ERROR : STORAGE_MISSING;
    }

    make_dir_if_missing("GAMES");
    make_dir_if_missing("SAVES");
    make_dir_if_missing("CONFIG");
    make_dir_if_missing("SYSTEM");

    s_mounted = true;
    return STORAGE_OK;
}

bool storage_is_mounted(void)
{
    return s_mounted;
}

storage_result_t storage_diag_roundtrip(void)
{
    static unsigned boot_nr = 0;
    boot_nr++;

    char payload[64];
    int n = snprintf(payload, sizeof(payload), "Teeny diag #%u phase0",
                     boot_nr);
    if (n <= 0) {
        return STORAGE_ERROR;
    }

    /* Write. */
    FIL f;
    FRESULT fr = f_open(&f, "SYSTEM/DIAG.TXT", FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK) {
        console_log("diag: open(w) failed (%d)", fr);
        return STORAGE_ERROR;
    }
    UINT written = 0;
    fr = f_write(&f, payload, (UINT)n, &written);
    const FRESULT fr_close_w = f_close(&f);
    if (fr != FR_OK || written != (UINT)n || fr_close_w != FR_OK) {
        console_log("diag: write failed (fr=%d w=%u wr=%u)", fr, written, fr_close_w);
        return STORAGE_ERROR;
    }

    /* Read back. */
    char back[64];
    fr = f_open(&f, "SYSTEM/DIAG.TXT", FA_READ);
    if (fr != FR_OK) {
        console_log("diag: open(r) failed (%d)", fr);
        return STORAGE_ERROR;
    }
    UINT got = 0;
    fr = f_read(&f, back, (UINT)sizeof(back) - 1, &got);
    const FRESULT fr_close_r = f_close(&f);
    back[got < sizeof(back) ? got : sizeof(back) - 1] = '\0';

    if (fr != FR_OK || fr_close_r != FR_OK) {
        console_log("diag: read failed (fr=%d)", fr);
        return STORAGE_ERROR;
    }
    if (got != (UINT)n || memcmp(payload, back, (size_t)n) != 0) {
        console_log("diag: mismatch (got=%u exp=%u)", got, n);
        return STORAGE_ERROR;
    }

    return STORAGE_OK;
}