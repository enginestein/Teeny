#include "teeny/kernel.h"

#include "teeny/version.h"
#include "teeny/board.h"
#include "teeny/console.h"
#include "teeny/usb_link.h"
#include "teeny/sd_spi.h"
#include "teeny/storage.h"
#include "teeny/display.h"
#include "teeny/video_out.h"
#include "teeny/input.h"
#include "teeny/mode.h"
#include "teeny/oled.h"
#include "teeny/game.h"
#include "teeny/launcher.h"
#include "teeny/snake.h"
#include "teeny/breakout.h"
#include "teeny/invaders.h"
#include "teeny/pong.h"
#include "teeny/tetris.h"
#include "teeny/runner.h"
#include "teeny/pacmate.h"
#include "teeny/flappy.h"

#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/gpio.h"

#include <stddef.h>

extern unsigned char _end;
extern unsigned char __StackLimit;

static const game_app_t s_games[] = {
    { "SNAKE",    snake_init,    snake_update,    snake_render,    snake_score },
    { "BREAKOUT", breakout_init, breakout_update, breakout_render, breakout_score },
    { "INVADERS", invaders_init, invaders_update, invaders_render, invaders_score },
    { "PONG",     pong_init,     pong_update,     pong_render,     pong_score },
    { "TETRIS",   tetris_init,   tetris_update,   tetris_render,   tetris_score },
    { "RUNNER",   runner_init,   runner_update,   runner_render,   runner_score },
    { "PACMATE",  pacmate_init,  pacmate_update,  pacmate_render,  pacmate_score },
    { "FLAPPY",   flappy_init,   flappy_update,   flappy_render,   flappy_score },
};
#define NUM_GAMES ((int)(sizeof(s_games) / sizeof(s_games[0])))

static const char *s_titles[NUM_GAMES];

typedef enum {
    APP_MENU = 0,
    APP_GAME = 1
} app_state_t;

typedef struct {
    uint32_t s_boot_count;
    uint32_t s_led_state;
    uint32_t s_last_status_ms;
    uint32_t s_last_tick_ms;
    uint32_t s_last_render_ms;
    uint32_t s_tick_acc_ms;
    unsigned long s_video_fps;
    unsigned long s_presented_last;
    app_state_t  s_state;
    int          s_active_game;
    bool         s_was_connected;
} kernel_t;

static kernel_t s_kernel;

static const char *sd_err_name(sd_error_t e)
{
    switch (e) {
    case SD_OK:            return "ok";
    case SD_ERR_NOT_PRESENT: return "not-present";
    case SD_ERR_TIMEOUT:   return "timeout";
    case SD_ERR_CRC:       return "crc";
    case SD_ERR_RANGE:     return "range";
    case SD_ERR_ILLEGAL:   return "illegal";
    case SD_ERR_IO:        return "io";
    default:               return "?";
    }
}

static uint32_t heap_free_bytes(void)
{
    return (uint32_t)((size_t)&__StackLimit - (size_t)&_end);
}

static void update_led(void)
{
    s_kernel.s_led_state = !s_kernel.s_led_state;
    gpio_put(BOARD_STATUS_LED_GPIO, s_kernel.s_led_state);
}

static void print_diagnostics(void)
{
    const sd_info_t *sd = sd_info();
    const usb_link_counters_t *lc = usb_link_counters();
    const uint32_t uptime_ms = to_ms_since_boot(get_absolute_time());
    const bool connected = usb_link_connected();
    const bool mounted = storage_is_mounted();

    console_status("boot count=%u", (unsigned)s_kernel.s_boot_count);
    console_status("uptime=%u ms", (unsigned)uptime_ms);
    console_status("usb: %s", connected ? "connected" : "detached");
    console_status("sd: present=%d type=%s err=%s cmd%d r1=0x%02X idle=0x%02X blocks=%llu",
                   sd->present ? 1 : 0,
                   sd->card_type == SD_CARD_HC ? "hc" : (sd->card_type == SD_CARD_STANDARD ? "sdsc" : "none"),
                   sd_err_name(sd->last_error), (int)sd->last_cmd,
                   sd->last_r1, sd->init_idle,
                   (unsigned long long)sd->block_count);
    console_status("fs: %s", mounted ? "mounted" : (sd->present ? "mount-error" : "no-volume"));
    console_status("heap free=%u bytes", (unsigned)heap_free_bytes());
    console_status("link: tx=%lu drop=%lu rx=%lu rxbytes=%lu",
                   lc->tx_frames, lc->tx_dropped, lc->rx_frames, lc->rx_bytes);

    const video_out_stats_t *vs = video_out_stats();
    console_status("video: %ux%u fb=%uB present=%lu skip=%lu abrt=%lu rows=%lu/%lu fps=%lu",
                   DISPLAY_WIDTH, DISPLAY_HEIGHT,
                   (unsigned)display_framebuffer_size_bytes(),
                   display_presented_frames(), display_skipped_presents(),
                   vs->frames_aborted, vs->rows_sent, vs->rows_dropped,
                   s_kernel.s_video_fps);

    if (s_kernel.s_state == APP_GAME) {
        const game_app_t *g = &s_games[s_kernel.s_active_game];
        console_status("app: %s score=%u", g->title, (unsigned)g->score());
    } else {
        console_status("app: menu sel=%d", launcher_selection());
    }
}

void kernel_run(void)
{
    s_kernel.s_boot_count++;

    gpio_init(BOARD_STATUS_LED_GPIO);
    gpio_set_dir(BOARD_STATUS_LED_GPIO, GPIO_OUT);
    gpio_put(BOARD_STATUS_LED_GPIO, 0);

    usb_link_init();

    const bool hw_oled = oled_probe();
    system_mode_set(hw_oled ? MODE_HARDWARE : MODE_REMOTE);

    if (g_system_mode == MODE_REMOTE) {
        console_boot("Teeny v%s (%s) core=%u reboot=%u remote",
                     TEENY_VERSION_STRING, TEENY_PHASE, get_core_num(),
                     (unsigned)s_kernel.s_boot_count);
        console_log("init: led=gpio%d sd=spi1:%u,%u,%u,%u",
                    BOARD_STATUS_LED_GPIO,
                    BOARD_SD_SCK_GPIO, BOARD_SD_MOSI_GPIO,
                    BOARD_SD_MISO_GPIO, BOARD_SD_CS_GPIO);
        display_init();
        video_out_init();
        console_log("init: fb=%ux%u rgb565 %u bytes",
                    DISPLAY_WIDTH, DISPLAY_HEIGHT,
                    (unsigned)display_framebuffer_size_bytes());
    } else {
        console_boot("Teeny v%s (%s) core=%u reboot=%u hardware",
                     TEENY_VERSION_STRING, TEENY_PHASE, get_core_num(),
                     (unsigned)s_kernel.s_boot_count);
        console_log("init: oled=ssd1306 sda=gpio%d scl=gpio%d addr=0x%02X",
                    BOARD_OLED_SDA_GPIO, BOARD_OLED_SCL_GPIO, BOARD_OLED_ADDR);
        display_init();
        console_log("init: oled=ssd1306 %ux%u 1-bit",
                    BOARD_OLED_WIDTH, BOARD_OLED_HEIGHT);
    }

    input_init();

    launcher_init();
    for (int i = 0; i < NUM_GAMES; i++) {
        s_titles[i] = s_games[i].title;
        s_games[i].init();
    }
    s_kernel.s_state = APP_MENU;
    s_kernel.s_active_game = 0;
    s_kernel.s_was_connected = false;
    console_log("init: %d games ready", NUM_GAMES);

    usb_link_update();
    sleep_ms(10);

    console_log("init: sd init...");
    const sd_error_t se = sd_init();
    if (se != SD_OK) console_log("init: sd %s", sd_err_name(se));
    usb_link_update();

    console_log("init: mount...");
    storage_result_t st = storage_init();
    (void)st;
    console_log("init: storage %s (result=%d)",
                storage_is_mounted() ? "ok" : "unavailable", (int)st);

    usb_link_update();

    if (storage_is_mounted()) {
        console_log("init: fs round-trip...");
        storage_result_t rt = storage_diag_roundtrip();
        console_status("diag roundtrip: %s", rt == STORAGE_OK ? "PASS" : "FAIL");
    }

    s_kernel.s_last_status_ms = to_ms_since_boot(get_absolute_time());
    s_kernel.s_last_tick_ms = s_kernel.s_last_status_ms;
    s_kernel.s_last_render_ms = s_kernel.s_last_status_ms;
    s_kernel.s_tick_acc_ms = 0;
    s_kernel.s_video_fps = 0;
    s_kernel.s_presented_last = 0;
    print_diagnostics();

    for (;;) {
        usb_link_update();

        if (g_system_mode == MODE_REMOTE) video_out_poll();

        const bool connected = usb_link_connected();
        if (connected && !s_kernel.s_was_connected) {
            s_kernel.s_state = APP_MENU;
            s_kernel.s_active_game = 0;
            launcher_init();
            console_log("link: cdc open -> launcher");
        }
        s_kernel.s_was_connected = connected;

        const uint32_t now = to_ms_since_boot(get_absolute_time());

        s_kernel.s_tick_acc_ms += now - s_kernel.s_last_tick_ms;
        s_kernel.s_last_tick_ms = now;
        if (s_kernel.s_tick_acc_ms > 100u) s_kernel.s_tick_acc_ms = 100u;
        while (s_kernel.s_tick_acc_ms >= 16u) {
            s_kernel.s_tick_acc_ms -= 16u;
            input_tick();
            if (s_kernel.s_state == APP_MENU) {
                launcher_update(NUM_GAMES);
                if (input_edge_pressed(BTN_A) || input_edge_pressed(BTN_START)) {
                    s_kernel.s_active_game = launcher_selection();
                    s_games[s_kernel.s_active_game].init();
                    s_kernel.s_state = APP_GAME;
                }
            } else {
                if (input_edge_pressed(BTN_SELECT)) {
                    s_kernel.s_state = APP_MENU;
                } else {
                    s_games[s_kernel.s_active_game].update();
                }
            }
        }

        if (!video_out_busy() && (now - s_kernel.s_last_render_ms) >= 33u) {
            s_kernel.s_last_render_ms = now;
            if (s_kernel.s_state == APP_MENU) {
                launcher_render(s_titles, NUM_GAMES);
            } else {
                s_games[s_kernel.s_active_game].render();
            }
            display_present();
        }

        if ((now - s_kernel.s_last_status_ms) >= 1000u) {
            s_kernel.s_last_status_ms = now;
            const unsigned long presented = display_presented_frames();
            s_kernel.s_video_fps = (unsigned long)(presented - s_kernel.s_presented_last);
            s_kernel.s_presented_last = presented;
            print_diagnostics();
            update_led();
        } else if ((now & 0xFFu) == 0u) {
            update_led();
        }

        sleep_ms(1);
    }
}
