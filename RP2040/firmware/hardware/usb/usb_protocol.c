/*
 * usb_protocol.c -- frame encode / receive parser for the Teeny protocol.
 *
 * See usb_protocol.h for the frame layout and recovery strategy.
 *
 * The checksum is a simple XOR over all preceding bytes. The USB CDC link
 * already guarantees packet integrity at the USB layer, so the checksum here
 * exists purely to detect framing errors (dropped/stray bytes) so the
 * receiver can resynchronise -- not as a transport integrity mechanism.
 */

#include "teeny/usb_protocol.h"

#include <string.h>

size_t usb_protocol_encode(uint8_t *buf, size_t buf_cap,
                           usb_packet_type_t type, uint8_t seq,
                           const uint8_t *payload, uint16_t payload_len)
{
    const size_t total = USB_PROTOCOL_HEADER_SIZE + (size_t)payload_len
                       + USB_PROTOCOL_CKSUM_SIZE;

    if (payload_len > USB_PROTOCOL_MAX_PAYLOAD || total > buf_cap) {
        return 0;
    }

    buf[0] = USB_PROTOCOL_MAGIC;
    buf[1] = (uint8_t)type;
    buf[2] = (uint8_t)((payload_len >> 8) & 0xffu);
    buf[3] = (uint8_t)(payload_len & 0xffu);
    buf[4] = seq;

    if (payload_len > 0 && payload != NULL) {
        memcpy(&buf[USB_PROTOCOL_HEADER_SIZE], payload, payload_len);
    }

    uint8_t cksum = 0;
    for (size_t i = 1; i < USB_PROTOCOL_HEADER_SIZE + (size_t)payload_len; i++) {
        cksum ^= buf[i];
    }
    buf[total - 1] = cksum;

    return total;
}

void usb_protocol_rx_init(usb_protocol_rx_t *rx)
{
    rx->used = 0;
    rx->expect = 0;
    rx->have_hdr = false;
}

/* Advance the parser by one byte. Returns true when the accumulated frame is
 * complete and VALID (checked before signalling completion). */
bool usb_protocol_rx_put(usb_protocol_rx_t *rx, uint8_t byte)
{
    /* A complete frame is waiting to be popped; do not consume more bytes
     * (would overflow the buffer). The caller must pop before refilling. */
    if (rx->have_hdr && rx->used >= rx->expect) {
        return true;
    }

    /* Resync: before anything else, the next byte must be MAGIC. */
    if (rx->used == 0 && !rx->have_hdr) {
        if (byte != USB_PROTOCOL_MAGIC) {
            return false;
        }
    }

    rx->buf[rx->used++] = byte;

    if (rx->used == USB_PROTOCOL_HEADER_SIZE && !rx->have_hdr) {
        /* Header is complete: compute the expected frame length. */
        uint16_t plen = ((uint16_t)rx->buf[2] << 8) | rx->buf[3];
        if (plen > USB_PROTOCOL_MAX_PAYLOAD) {
            /* Impossible length declared -- frame is garbage. Drop it and
             * resync by looking for the next MAGIC. */
            rx->used = 0;
            return false;
        }
        rx->expect = (uint16_t)(USB_PROTOCOL_HEADER_SIZE + plen + USB_PROTOCOL_CKSUM_SIZE);
        rx->have_hdr = true;
    }

    if (rx->have_hdr && rx->used >= rx->expect) {
        /* Whole frame buffered; verify checksum. */
        uint8_t cksum = 0;
        for (uint16_t i = 1; i + 1 < rx->expect; i++) {
            cksum ^= rx->buf[i];
        }
        if (cksum != rx->buf[rx->expect - 1]) {
            /* Bad checksum: resync from scratch. */
            rx->used = 0;
            rx->have_hdr = false;
            return false;
        }
        return true;
    }

    return false;
}

bool usb_protocol_rx_pop(usb_protocol_rx_t *rx, usb_packet_type_t *type,
                         uint8_t *payload, uint16_t *payload_len, uint16_t cap)
{
    if (!rx->have_hdr || rx->used < rx->expect) {
        return false;
    }

    const uint16_t plen = ((uint16_t)rx->buf[2] << 8) | rx->buf[3];

    if (type != NULL) {
        *type = (usb_packet_type_t)rx->buf[1];
    }

    const uint16_t to_copy = plen < cap ? plen : cap;
    if (payload != NULL && to_copy > 0) {
        memcpy(payload, &rx->buf[USB_PROTOCOL_HEADER_SIZE], to_copy);
    }
    if (payload_len != NULL) {
        *payload_len = plen; /* report actual length, host may truncate */
    }

    rx->used = 0;
    rx->have_hdr = false;
    return true;
}