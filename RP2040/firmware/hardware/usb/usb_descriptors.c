/*
 * usb_descriptors.c -- USB device descriptors for Teeny.
 *
 * The device presents a single CDC (serial) class device carrying the framed
 * Teeny protocol. VID 0x2e8a is the Raspberry Pi vendor ID, consistent with
 * the RP2040 hardware; PID 0x00a0 is the Teeny-specific product id used by
 * the host tool to find the console.
 */

#include "tusb_config.h"
#include "tusb.h"

#include "pico/unique_id.h"

#include <string.h>

#define TEENY_VID 0x2e8a
#define TEENY_PID 0x00a0
#define TEENY_BCD_DEVICE 0x0300

enum {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
    STRID_CDC,
};

enum {
    ITF_CDC = 0,
    ITF_CDC_DATA,
    ITF_COUNT
};

/* Endpoint numbers. IN = device-to-host, OUT = host-to-device. */
enum {
    EPNUM_CDC_NOTIF = 0x81,
    EPNUM_CDC_OUT = 0x02,
    EPNUM_CDC_IN  = 0x82,
};

static const tusb_desc_device_t desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_CDC,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = TEENY_VID,
    .idProduct          = TEENY_PID,
    .bcdDevice          = TEENY_BCD_DEVICE,
    .iManufacturer      = STRID_MANUFACTURER,
    .iProduct           = STRID_PRODUCT,
    .iSerialNumber      = STRID_SERIAL,
    .bNumConfigurations = 1,
};

static const uint8_t desc_fs_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT, 0,
                          TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN,
                          0x80 /* bus powered */, 100),
    TUD_CDC_DESCRIPTOR(ITF_CDC, STRID_CDC, EPNUM_CDC_NOTIF, 8,
                       EPNUM_CDC_OUT, EPNUM_CDC_IN,
                       CFG_TUD_CDC_EP_BUFSIZE),
};

static const char *const string_desc_arr[] = {
    (const char[]){0x09, 0x04}, /* 0: supported languages (English) */
    "Teeny",                   /* 1: manufacturer */
    "Teeny RP2040 Console",    /* 2: product */
    NULL,                       /* 3: serial (generated at runtime) */
    "Teeny Console",           /* 4: CDC interface */
};

const uint8_t *tud_descriptor_device_cb(void)
{
    return (const uint8_t *)&desc_device;
}

const uint8_t *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return desc_fs_configuration;
}

/* Serial number: the RP2040's 8-byte unique id rendered as 16 hex chars,
 * same convention as other RP2040-based devices on the host. */
static const char *build_serial_string(void)
{
    static char serial[17];
    static bool built = false;

    if (!built) {
        pico_unique_board_id_t id;
        pico_get_unique_board_id(&id);
        for (unsigned i = 0; i < sizeof(id.id); i++) {
            const char hex[] = "0123456789ABCDEF";
            serial[i * 2]     = hex[(id.id[i] >> 4) & 0xf];
            serial[i * 2 + 1] = hex[id.id[i] & 0xf];
        }
        serial[16] = '\0';
        built = true;
    }
    return serial;
}

/* String descriptor callback (TinyUSB v0.16+: returns uint16_t const*).
 * The serial number is the RP2040's 8-byte unique id as 16 hex chars, the
 * same convention as other RP2040 devices on the host. */
static uint16_t s_desc_str[64];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;
    unsigned chr_count = 0;

    if (index == STRID_SERIAL) {
        const char *serial = build_serial_string();
        chr_count = 16;
        for (unsigned i = 0; i < chr_count; i++) {
            s_desc_str[1 + i] = (uint16_t)serial[i];
        }
    } else if (index == STRID_LANGID) {
        memcpy(&s_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else if (index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) {
        const char *str = string_desc_arr[index];
        if (str == NULL) {
            return NULL;
        }
        const unsigned max_chars = 63 - 1; /* room for length + type */
        while (str[chr_count] && chr_count < max_chars) {
            s_desc_str[1 + chr_count] = (uint16_t)str[chr_count];
            chr_count++;
        }
    } else {
        return NULL;
    }

    s_desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
    return s_desc_str;
}