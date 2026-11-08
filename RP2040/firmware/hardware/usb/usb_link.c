#include "tusb_config.h"
#include "tusb.h"

#include "teeny/usb_link.h"
#include "teeny/usb_protocol.h"
#include "teeny/console.h"
#include "teeny/input.h"

#include "pico/bootrom.h"
#include "pico/stdlib.h"

#include <string.h>

static usb_protocol_rx_t s_rx;
static uint8_t s_tx_seq;
static usb_link_counters_t s_counters;

#define REPLAY_SLOTS 8u
#define REPLAY_FRAME_MAX 96u
static uint8_t s_replay[REPLAY_SLOTS][REPLAY_FRAME_MAX];
static uint8_t s_replay_len[REPLAY_SLOTS];
static uint8_t s_replay_head;
static bool s_was_connected;

static bool is_text_packet(usb_packet_type_t t)
{
    return t == PKT_BOOT || t == PKT_STATUS || t == PKT_LOG;
}

static void replay_store(const uint8_t *frame, size_t len)
{
    if (!is_text_packet((usb_packet_type_t)frame[1]) || len > REPLAY_FRAME_MAX) return;
    memcpy(s_replay[s_replay_head], frame, len);
    s_replay_len[s_replay_head] = (uint8_t)len;
    s_replay_head = (uint8_t)((s_replay_head + 1) % REPLAY_SLOTS);
}

static void replay_send_all(void)
{
    for (uint8_t i = 0; i < REPLAY_SLOTS; i++) {
        const uint8_t idx = (uint8_t)((s_replay_head + i) % REPLAY_SLOTS);
        if (s_replay_len[idx] == 0) continue;
        if (tud_cdc_write_available() < s_replay_len[idx]) break;
        tud_cdc_write(s_replay[idx], s_replay_len[idx]);
    }
}

void usb_link_init(void)
{
    s_tx_seq = 0;
    usb_protocol_rx_init(&s_rx);
    tusb_init();
}

bool usb_link_connected(void)
{
    return tud_cdc_connected();
}

unsigned usb_link_tx_available(void)
{
    const int avail = tud_cdc_write_available();
    return avail > 0 ? (unsigned)avail : 0u;
}

bool usb_link_send_packet(usb_packet_type_t type,
                          const uint8_t *payload, uint16_t payload_len)
{
    uint8_t frame[USB_PROTOCOL_HEADER_SIZE + USB_PROTOCOL_MAX_PAYLOAD +
                  USB_PROTOCOL_CKSUM_SIZE];

    const size_t frame_len = usb_protocol_encode(frame, sizeof(frame), type,
                                                 s_tx_seq++, payload, payload_len);
    if (frame_len == 0) {
        s_counters.tx_dropped++;
        return false;
    }

    if (!tud_ready() || !tud_cdc_connected()) {
        s_counters.tx_dropped++;
        return false;
    }
    if (tud_cdc_write_available() < frame_len) {
        s_counters.tx_dropped++;
        return false;
    }

    tud_cdc_write(frame, frame_len);
    replay_store(frame, frame_len);
    s_counters.tx_frames++;
    return true;
}

bool usb_link_send_text(usb_packet_type_t type, const char *text, uint16_t len)
{
    return usb_link_send_packet(type, (const uint8_t *)text, len);
}

const usb_link_counters_t *usb_link_counters(void)
{
    return &s_counters;
}

static void handle_rx_frame(usb_packet_type_t type,
                            const uint8_t *payload, uint16_t len)
{
    if (type == PKT_CMD_REQ) {
        if (len == 3 && memcmp(payload, "dfu", 3) == 0) {
            console_log("rx: RESET request (dfu)");
            usb_link_send_packet(PKT_RESET, payload, len);
            tud_cdc_write_flush();
            sleep_ms(20);
            reset_usb_boot(0, 0);
            return;
        }
        usb_link_send_packet(PKT_CMD_RSP, payload, len);
        console_log("rx: cmd len=%u (echo)", len);
        return;
    }
    if (type == PKT_INPUT) {
        input_set_mask(len >= 1 ? payload[0] : 0u);
        return;
    }
}

static void drain_rx(void)
{
    while (tud_cdc_available() > 0) {
        uint8_t b;
        if (tud_cdc_read(&b, 1) != 1) break;
        s_counters.rx_bytes++;
        if (usb_protocol_rx_put(&s_rx, b)) {
            usb_packet_type_t type;
            uint8_t payload[USB_PROTOCOL_MAX_PAYLOAD];
            uint16_t plen;
            if (usb_protocol_rx_pop(&s_rx, &type, payload, &plen, sizeof(payload))) {
                s_counters.rx_frames++;
                handle_rx_frame(type, payload, plen);
            }
        }
    }
}

void usb_link_update(void)
{
    tud_task();

    const bool connected = tud_cdc_connected();
    if (connected && !s_was_connected) {
        replay_send_all();
    }
    s_was_connected = connected;

    if (tud_cdc_write_available() < CFG_TUD_CDC_TX_BUFSIZE) {
        tud_cdc_write_flush();
    }

    drain_rx();
}
