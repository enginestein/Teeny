/*-----------------------------------------------------------------------/
/  Low level disk interface glue: Teeny SD (SPI) -> FatFs               /
/-----------------------------------------------------------------------*/

#include "ff.h"      /* Obtains integer types */
#include "diskio.h"  /* Declarations of disk functions */

#include "teeny/sd_spi.h"

/* FatFs drive 0 is the on-board microSD. */
#define SD_DRIVE 0u

/* No card-detect GPIO is wired on this board (the SparkFun Thing Plus SD
 * socket has no CD line routed to the RP2040). Media-presence is therefore
 * determined by whether the card answered during sd_init(). */
static DSTATUS s_status = STA_NOINIT;

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != SD_DRIVE) {
        return STA_NOINIT;
    }

    sd_error_t e = sd_init();
    if (e == SD_OK) {
        s_status = 0;
    } else if (e == SD_ERR_NOT_PRESENT) {
        s_status = STA_NOINIT | STA_NODISK;
    } else {
        s_status = STA_NOINIT;
    }
    return s_status;
}

DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != SD_DRIVE) {
        return STA_NOINIT;
    }
    return s_status;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != SD_DRIVE) {
        return RES_PARERR;
    }
    if (s_status & (STA_NOINIT | STA_NODISK)) {
        return RES_NOTRDY;
    }
    if (count == 0) {
        return RES_PARERR;
    }

    sd_error_t e = sd_read_sectors((uint32_t)sector, buff, (uint32_t)count);
    if (e == SD_OK) {
        return RES_OK;
    }
    if (e == SD_ERR_NOT_PRESENT) {
        s_status |= STA_NODISK;
    }
    return RES_ERROR;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != SD_DRIVE) {
        return RES_PARERR;
    }
    if (s_status & (STA_NOINIT | STA_NODISK)) {
        return RES_NOTRDY;
    }
    if (s_status & STA_PROTECT) {
        return RES_WRPRT;
    }
    if (count == 0) {
        return RES_PARERR;
    }

    sd_error_t e = sd_write_sectors((uint32_t)sector, buff, (uint32_t)count);
    if (e == SD_OK) {
        return RES_OK;
    }
    if (e == SD_ERR_NOT_PRESENT) {
        s_status |= STA_NODISK;
    }
    return RES_ERROR;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv != SD_DRIVE) {
        return RES_PARERR;
    }

    switch (cmd) {
    case CTRL_SYNC:
        /* The SPI driver completes and de-asserts CS on every operation;
         * nothing remains to flush. */
        return RES_OK;

    case GET_SECTOR_COUNT: {
        const sd_info_t *info = sd_info();
        *(LBA_t *)buff = info->block_count;
        return RES_OK;
    }

    case GET_SECTOR_SIZE:
        *(WORD *)buff = 512u;
        return RES_OK;

    case GET_BLOCK_SIZE:
        *(DWORD *)buff = 1u; /* not used (FF_USE_MKFS == 0) */
        return RES_OK;

    default:
        return RES_PARERR;
    }
}