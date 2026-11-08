#ifndef TEENY_VIDEO_OUT_H
#define TEENY_VIDEO_OUT_H

#include <stdbool.h>
#include <stdint.h>

#include "teeny/usb_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

void video_out_init(void);
bool video_out_start(const uint8_t *pixels);
bool video_out_busy(void);
void video_out_poll(void);

typedef struct {
    unsigned long frames_sent;
    unsigned long frames_aborted;
    unsigned long rows_sent;
    unsigned long rows_dropped;
} video_out_stats_t;

const video_out_stats_t *video_out_stats(void);

#ifdef __cplusplus
}
#endif

#endif /* TEENY_VIDEO_OUT_H */
