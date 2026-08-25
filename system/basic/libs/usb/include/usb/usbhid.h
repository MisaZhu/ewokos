/*
 * usbhid.h: hardware-independent USB HID report-descriptor parsing.
 *
 * Shared by every usbhostd implementation: given a raw report descriptor
 * it classifies the device (keyboard/mouse/touch), extracts the bit
 * layout of mouse/touch reports, and normalizes incoming reports into
 * the fixed /dev/hid0 event formats. No controller or transfer code
 * lives here.
 */
#ifndef __USBHID_H__
#define __USBHID_H__

#include <stdint.h>
#include <stdbool.h>
#include <usb/usb_defs.h>

typedef enum {
    HID_DEV_TYPE_UNKNOWN = 0,
    HID_DEV_TYPE_KEYBOARD,
    HID_DEV_TYPE_MOUSE,
    HID_DEV_TYPE_TOUCH,
} hid_dev_type_t;

typedef struct {
    bool valid;
    bool has_report_id;
    uint8_t report_id;
    uint8_t report_bytes;
    int tip_bit;
    int tip_size;
    int x_bit;
    int x_size;
    int y_bit;
    int y_size;
    uint32_t x_max;
    uint32_t y_max;
} touch_parser_t;

typedef struct {
    bool valid;
    bool has_report_id;
    uint8_t report_id;
    uint8_t report_bytes;
    int button_bit[3];
    int button_size[3];
    int x_bit;
    int x_size;
    int y_bit;
    int y_size;
    int wheel_bit;
    int wheel_size;
} mouse_parser_t;

/* one HID interface candidate found while walking a config descriptor */
typedef struct {
    bool valid;
    uint8_t iface_num;
    uint8_t subclass;
    uint8_t protocol;
    uint8_t ep_addr;
    uint8_t interval; /* raw bInterval: the host layer decides how to use it */
    uint16_t max_packet;
    uint16_t report_desc_len;
} hid_candidate_t;

/* classify a device from its report descriptor */
hid_dev_type_t hid_detect_device_type(const uint8_t* desc, int len);

/* composite detection: returns 0 with both IDs filled when the descriptor
   holds a keyboard AND a mouse/pointer application collection each with
   its own report ID */
int hid_parse_report_ids(const uint8_t* desc, int len,
        uint8_t* kbd_id, uint8_t* mouse_id);

/* Report ID of the keyboard application collection, or 0 when the
   keyboard collection has no Report ID (plain boot layout) */
uint8_t hid_find_kbd_report_id(const uint8_t* desc, int len);

/* extract the bit layout of touch/mouse reports from a descriptor */
int hid_parse_touch_report(const uint8_t* desc, int len, touch_parser_t* out);
bool hid_probe_touch_report(const uint8_t* desc, int len, touch_parser_t* out);
int hid_parse_mouse_report(const uint8_t* desc, int len, mouse_parser_t* out);
bool hid_probe_mouse_report(const uint8_t* desc, int len, mouse_parser_t* out);

/* reject implausible parser results before trusting them; strict is used
   for boot interfaces that can fall back to the boot layout instead */
bool mouse_parser_sane(const mouse_parser_t* p, uint16_t max_packet, bool strict);

/* normalize one raw report into a USB_POINTER_EVENT_SIZE byte event.
   Returns the event size or -1 when the report does not match the parser.
   touch_normalize_report additionally takes the endpoint's expected
   report length (variable-size reports arrive as full packets). */
int mouse_normalize_report(const mouse_parser_t* m,
        const uint8_t* report, int len, uint8_t* out);
int touch_normalize_report(const touch_parser_t* t, uint8_t report_len,
        const uint8_t* report, int len, uint8_t* out);

#endif /* __USBHID_H__ */
