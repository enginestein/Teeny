#ifndef TEENY_CONSOLE_H
#define TEENY_CONSOLE_H

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void console_log(const char *fmt, ...);
void console_status(const char *fmt, ...);
void console_boot(const char *fmt, ...);
void console_log_v(const char *fmt, va_list args);

typedef struct {
    unsigned long frames_sent;
    unsigned long frames_dropped;
} console_stats_t;

const console_stats_t *console_stats(void);

#ifdef __cplusplus
}
#endif

#endif /* TEENY_CONSOLE_H */
