#include "teeny/console.h"
#include "teeny/usb_protocol.h"
#include "teeny/usb_link.h"

#include <stdio.h>

static console_stats_t s_stats;

static void emit(usb_packet_type_t type, const char *fmt, va_list args)
{
    char buf[USB_PROTOCOL_MAX_PAYLOAD];
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    if (n <= 0) return;
    if (n >= (int)sizeof(buf)) n = (int)sizeof(buf) - 1;
    s_stats.frames_sent++;
    usb_link_send_text(type, buf, (uint16_t)n);
}

void console_log_v(const char *fmt, va_list args)
{
    emit(PKT_LOG, fmt, args);
}

void console_log(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    emit(PKT_LOG, fmt, args);
    va_end(args);
}

void console_status(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    emit(PKT_STATUS, fmt, args);
    va_end(args);
}

void console_boot(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    emit(PKT_BOOT, fmt, args);
    va_end(args);
}

const console_stats_t *console_stats(void)
{
    return &s_stats;
}
