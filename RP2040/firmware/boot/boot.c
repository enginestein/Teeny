#include "teeny/boot.h"
#include "teeny/version.h"
#include "teeny/board.h"
#include "teeny/console.h"

#include "pico/types.h"
#include "hardware/clocks.h"

void boot_print_banner(void)
{
    const unsigned mhz = clock_get_hz(clk_sys) / 1000000u;
    const uint32_t flash_kb = 16u * 1024u; /* fixed by board (sparkfun_thingplus) */

    console_boot("%s v%s  (Phase %s)", TEENY_NAME, TEENY_VERSION_STRING, TEENY_PHASE);
    console_boot("Board   : SparkFun Thing Plus RP2040");
    console_boot("CPU     : %u MHz", mhz);
    console_boot("SRAM    : 264 KB");
    console_boot("Flash   : %u MB (%u KB)", flash_kb / 1024u, flash_kb);
    console_boot("SD slot : SPI1 SCK=GP%d MOSI=GP%d MISO=GP%d CS=GP%d",
                  BOARD_SD_SCK_GPIO, BOARD_SD_MOSI_GPIO, BOARD_SD_MISO_GPIO, BOARD_SD_CS_GPIO);
}