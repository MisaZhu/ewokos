/*
 * hid_report.h: transport-independent HID report-descriptor parsing.
 *
 * Given a raw report descriptor - from USB GET_DESCRIPTOR(report), from
 * a classic-Bluetooth SDP HIDDescriptorList, or from the HOGP Report Map
 * characteristic - this classifies the device (keyboard/mouse/touch),
 * extracts the bit layout of mouse/touch reports and normalizes incoming
 * reports into the fixed subscriber event formats.
 *
 * No transport, controller or descriptor-fetching code lives here.
 */
#ifndef __HID_REPORT_H__
#define __HID_REPORT_H__

#include <stdint.h>
#include <stdbool.h>
#include <hid/hid_defs.h>
#include <hid/hid_joystick.h>

typedef enum {
    HID_DEV_TYPE_UNKNOWN = 0,
    HID_DEV_TYPE_KEYBOARD,
    HID_DEV_TYPE_MOUSE,
    HID_DEV_TYPE_TOUCH,
    HID_DEV_TYPE_JOYSTICK,
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
    /* X/Y declared with the HID Input "Relative" bit set: a genuine relative
       mouse/touchpad. Distinguishes a high-resolution 16-bit relative mouse
       from an absolute touchscreen, which axis width alone cannot do. */
    bool axis_relative;
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

/* One analog field: its bit offset/width plus the logical range declared in
   the descriptor, so normalize can scale min..max onto the js_evt_t range. */
typedef struct {
    bool present;
    int bit;
    int size;
    int32_t logical_min;
    int32_t logical_max;
} js_axis_t;

/* Generic joystick/gamepad bit layout, extracted from the report descriptor
   the same way mouse_parser_t/touch_parser_t are. Handles the whole family
   of descriptor-driven USB gamepads (the uConsole's USBComposite joystick,
   standard HID gamepads, ...) without a per-device byte table: buttons are
   a contiguous 1-bit-per-usage field, the hat is a 4-bit switch, and the
   axes/sliders are wide absolute fields scaled by their logical range. */
#define JS_MAX_BUTTONS 32
#define JS_AXIS_COUNT 6 /* X, Y, Z, Rx, Ry, Rz */

/* Button mapping profiles. Different gamepads use different HID Button
   usage orders for the face-button diamond. The parser latches one of
   these based on device name or descriptor hints. */
typedef enum {
    JS_MAP_DEFAULT = 0,  /* 1=X 2=A 3=B 4=Y (uConsole, generic HID) */
    JS_MAP_XBOX,         /* 1=A 2=B 3=X 4=Y (Xbox standard) */
} js_map_type_t;

typedef struct {
    bool valid;
    bool has_report_id;
    uint8_t report_id;
    uint8_t report_bytes;
    /* buttons: a run of button_count fields of button_size bits starting at
       button_bit; button_usage_min is the HID Button usage of the first */
    int button_bit;
    int button_size;
    int button_count;
    uint32_t button_usage_min;
    /* hat switch (Generic Desktop 0x39): 0..7 compass, > 7 released */
    int hat_bit;
    int hat_size;
    int32_t hat_min;
    int32_t hat_max;
    /* axes indexed 0..5 = X, Y, Z, Rx, Ry, Rz */
    js_axis_t axis[JS_AXIS_COUNT];
    /* up to two sliders (Generic Desktop 0x36), used as analog triggers */
    js_axis_t slider[2];
    /* button mapping profile, set by the caller after parsing */
    js_map_type_t map_type;
} joystick_parser_t;


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

/* extract the bit layout of a joystick/gamepad report from a descriptor */
int hid_parse_joystick_report(const uint8_t* desc, int len, joystick_parser_t* out);
bool hid_probe_joystick_report(const uint8_t* desc, int len, joystick_parser_t* out);

/* reject implausible parser results before trusting them; strict is used
   for boot interfaces that can fall back to the boot layout instead */
bool mouse_parser_sane(const mouse_parser_t* p, uint16_t max_packet, bool strict);

/* normalize one raw report into a HID_POINTER_EVENT_SIZE byte event.
   Returns the event size or -1 when the report does not match the parser.
   touch_normalize_report additionally takes the endpoint's expected
   report length (variable-size reports arrive as full packets). */
int mouse_normalize_report(const mouse_parser_t* m,
        const uint8_t* report, int len, uint8_t* out);
int touch_normalize_report(const touch_parser_t* t, uint8_t report_len,
        const uint8_t* report, int len, uint8_t* out);

/* Normalize a raw joystick/gamepad report straight into a js_evt_t using the
   descriptor-driven layout. Returns 0 on success (out->connected set) or -1
   when the report does not match the parser. */
int joystick_normalize_report(const joystick_parser_t* j,
        const uint8_t* report, int len, js_evt_t* out);

/* Normalize an absolute-coordinate mouse report into touch-event format.
   Many USB touchscreens present as Generic Desktop/Mouse with absolute
   X/Y (>=12 bits) instead of a proper Digitizer collection. This extracts
   button[0] as tip-switch and the raw X/Y as absolute position.
   Returns HID_POINTER_EVENT_SIZE on success, -1 on failure. */
int mouse_normalize_as_touch(const mouse_parser_t* m,
        const uint8_t* report, int len, uint8_t* out);

/* True when the mouse parser looks like an absolute pointing device
   (touchscreen) rather than a relative mouse: both axes > 8 bits. */
bool mouse_parser_is_absolute(const mouse_parser_t* m);

#endif /* __HID_REPORT_H__ */
