#ifndef TEENY_USB_PROTOCOL_H
#define TEENY_USB_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USB_PROTOCOL_MAGIC       0x47u
#define USB_PROTOCOL_HEADER_SIZE 5u
#define USB_PROTOCOL_CKSUM_SIZE  1u
#define USB_PROTOCOL_MAX_PAYLOAD 512u

typedef enum {
    PKT_NONE = 0x00,
    PKT_BOOT      = 0x01,
    PKT_STATUS    = 0x02,
    PKT_LOG       = 0x03,
    PKT_CMD_REQ   = 0x10,
    PKT_CMD_RSP   = 0x11,
    PKT_FRAME     = 0x20,
    PKT_INPUT     = 0x21,
    PKT_FPS       = 0x22,
    PKT_FRAME_END = 0x23,
    PKT_FRAME_RLE = 0x24,
    PKT_RESET     = 0x30,
} usb_packet_type_t;

typedef struct {
    uint8_t  payload[USB_PROTOCOL_MAX_PAYLOAD];
    uint16_t payload_len;
    uint8_t  seq;
} usb_packet_t;

size_t usb_protocol_encode(uint8_t *buf, size_t buf_cap,
                           usb_packet_type_t type, uint8_t seq,
                           const uint8_t *payload, uint16_t payload_len);

typedef struct {
    uint8_t  buf[USB_PROTOCOL_HEADER_SIZE + USB_PROTOCOL_MAX_PAYLOAD + USB_PROTOCOL_CKSUM_SIZE];
    uint16_t used;
    uint16_t expect;
    bool     have_hdr;
} usb_protocol_rx_t;

void usb_protocol_rx_init(usb_protocol_rx_t *rx);
bool usb_protocol_rx_put(usb_protocol_rx_t *rx, uint8_t byte);
bool usb_protocol_rx_pop(usb_protocol_rx_t *rx, usb_packet_type_t *type,
                         uint8_t *payload, uint16_t *payload_len, uint16_t cap);

#ifdef __cplusplus
}
#endif

#endif /* TEENY_USB_PROTOCOL_H */
