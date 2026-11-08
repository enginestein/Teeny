/*
 * tusb_config.h -- TinyUSB build configuration for the Teeny device.
 *
 * CFG_TUSB_MCU / CFG_TUSB_OS / CFG_TUSB_DEBUG are supplied by the
 * pico-sdk tinyusb integration (family.cmake), so they are intentionally
 * not redefined here.
 *
 * CDC buffers must be large enough to hold one maximum-size frame
 * (HEADER + MAX_PAYLOAD + CKSUM = 5 + 512 + 1 = 518 bytes). usb_link sends
 * whole frames into the TinyUSB CDC TX FIFO, so the TX FIFO must fit a
 * frame; otherwise long payloads would be dropped needlessly.
 */

#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE)

#define CFG_TUSB_DEBUG 0

/* Maximum length of descriptor in the endpoint 0 / control transfers. */
#define CFG_TUD_ENDPOINT0_SIZE 64

/* One CDC (the Teeny protocol pipe). */
#define CFG_TUD_CDC 1

/* Buffers must fit one maximum-size protocol frame (5 + 512 + 1 = 518 B).
 * The TX buffer also carries bursts of framebuffer rows; video_usb writes as
 * many rows as there is free space (drain-adaptive), so a larger buffer lets
 * ~12 scanlines queue behind a slow host while staying a small RAM cost. */
#define CFG_TUD_CDC_RX_BUFSIZE 640
#define CFG_TUD_CDC_TX_BUFSIZE 4096

/* The RP2040 is a USB Full-Speed device; bulk endpoint max is 64 bytes. */
#define CFG_TUD_CDC_EP_BUFSIZE 64

#endif /* TUSB_CONFIG_H */