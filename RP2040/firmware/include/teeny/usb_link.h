/*
 * usb_link.h -- USB transport for the Teeny protocol.
 *
 * This is the hardware (TinyUSB/CDC) half of the development link. The
 * protocol framing lives in usb_protocol.c; usb_link is the only module that
 * knows TinyUSB exists, and the only place that touches the CDC endpoints.
 *
 * Sending is best-effort and non-blocking. Frames are dropped (and counted)
 * if the PC is not connected or the CDC TX FIFO cannot take a whole frame.
 * This is deliberate: game timing must never stall on USB backpressure.
 */

#ifndef TEENY_USB_LINK_H
#define TEENY_USB_LINK_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "teeny/usb_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

void usb_link_init(void);

/* Pump the USB stack (TinyUSB task), flush TX, drain RX. Call frequently
 * from the main loop. */
void usb_link_update(void);

/* True once the host has opened the CDC port (DTR asserted). */
bool usb_link_connected(void);

/* Free space in the CDC TX FIFO in bytes. Consumers (e.g. the framebuffer
 * transport) use this to pace writes instead of hammering the FIFO. */
unsigned usb_link_tx_available(void);

/* Send one framed packet. Returns true if it was accepted into the TX FIFO. */
bool usb_link_send_packet(usb_packet_type_t type,
                          const uint8_t *payload, uint16_t payload_len);

/* Convenience for text payloads (boot/log/status). */
bool usb_link_send_text(usb_packet_type_t type, const char *text, uint16_t len);

typedef struct {
    unsigned long tx_frames;   /* frames accepted into the TX FIFO */
    unsigned long tx_dropped;  /* frames discarded (not connected / full) */
    unsigned long rx_frames;   /* valid frames received from the host */
    unsigned long rx_bytes;    /* raw bytes received              */
} usb_link_counters_t;

const usb_link_counters_t *usb_link_counters(void);

#ifdef __cplusplus
}
#endif

#endif /* TEENY_USB_LINK_H */