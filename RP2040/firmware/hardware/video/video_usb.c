#include "teeny/video_out.h"

#include "teeny/display.h"
#include "teeny/usb_link.h"

#include <string.h>

#define ROW_BYTES            (DISPLAY_WIDTH * 2u)
#define ROW_PACKET_PAYLOAD   (1u + ROW_BYTES)
#define ROW_PACKET_TOTAL     (USB_PROTOCOL_HEADER_SIZE + ROW_PACKET_PAYLOAD \
                             + USB_PROTOCOL_CKSUM_SIZE)
#define RLE_ROW_MAX          (1u + 1u + ROW_BYTES)

static uint16_t rle_encode_body(const display_color_t *line,
                                uint8_t *out, size_t out_cap)
{
    size_t i = 0;
    size_t n = 0;
    while (i < DISPLAY_WIDTH) {
        size_t run = 1;
        while (i + run < DISPLAY_WIDTH && line[i + run] == line[i]) run++;
        if (run >= 2) {
            while (run > 0) {
                const size_t chunk = run > 127u ? 127u : run;
                if (n + 3 > out_cap) return 0;
                out[n++] = (uint8_t)chunk;
                out[n++] = (uint8_t)(line[i] & 0xFFu);
                out[n++] = (uint8_t)(line[i] >> 8);
                i += chunk;
                run -= chunk;
            }
        } else {
            size_t len = 1;
            while (i + len < DISPLAY_WIDTH &&
                   (len >= 128u || line[i + len] != line[i + len - 1])) len++;
            if (len > 128u) len = 128u;
            if (n + 1 + len * 2 > out_cap) return 0;
            out[n++] = (uint8_t)(0x80u | (len - 1));
            for (size_t k = 0; k < len; k++) {
                out[n++] = (uint8_t)(line[i + k] & 0xFFu);
                out[n++] = (uint8_t)(line[i + k] >> 8);
            }
            i += len;
        }
    }
    return (uint16_t)n;
}

static const uint8_t *s_pixels;
static uint8_t s_row;
static uint16_t s_frame_id;
static bool s_busy;
static video_out_stats_t s_stats;

void video_out_init(void)
{
    s_pixels = NULL;
    s_row = 0;
    s_frame_id = 0;
    s_busy = false;
    memset(&s_stats, 0, sizeof(s_stats));
}

bool video_out_busy(void)
{
    return s_busy;
}

bool video_out_start(const uint8_t *pixels)
{
    if (s_busy) return false;
    s_pixels = pixels;
    s_row = 0;
    s_busy = true;
    s_stats.frames_sent++;
    return true;
}

void video_out_poll(void)
{
    if (!s_busy || s_pixels == NULL) return;

    while (usb_link_tx_available() >= ROW_PACKET_TOTAL) {
        if (s_row >= DISPLAY_HEIGHT) {
            uint8_t end[3];
            end[0] = (uint8_t)(s_frame_id & 0xFFu);
            end[1] = (uint8_t)DISPLAY_WIDTH;
            end[2] = (uint8_t)DISPLAY_HEIGHT;
            s_frame_id++;
            if (!usb_link_send_packet(PKT_FRAME_END, end, sizeof(end))) {
                s_stats.rows_dropped++;
                return;
            }
            s_busy = false;
            s_pixels = NULL;
            return;
        }

        uint8_t payload[ROW_PACKET_PAYLOAD];
        payload[0] = (uint8_t)s_row;
        const display_color_t *line =
            (const display_color_t *)&s_pixels[(size_t)s_row * ROW_BYTES];

        uint8_t rle[RLE_ROW_MAX];
        const uint16_t rle_body = rle_encode_body(line, rle, sizeof(rle));
        bool ok;
        if (rle_body != 0 && (size_t)1 + rle_body < ROW_BYTES) {
            memcpy(&payload[1], rle, rle_body);
            ok = usb_link_send_packet(PKT_FRAME_RLE, payload,
                                      (uint16_t)(1 + rle_body));
        } else {
            for (int i = 0; i < DISPLAY_WIDTH; i++) {
                const display_color_t px = line[i];
                payload[1 + (size_t)i * 2 + 0] = (uint8_t)(px & 0xFFu);
                payload[1 + (size_t)i * 2 + 1] = (uint8_t)(px >> 8);
            }
            ok = usb_link_send_packet(PKT_FRAME, payload,
                                      (uint16_t)sizeof(payload));
        }

        if (!ok) {
            s_stats.rows_dropped++;
            return;
        }
        s_stats.rows_sent++;
        s_row++;
    }
}

const video_out_stats_t *video_out_stats(void)
{
    return &s_stats;
}
