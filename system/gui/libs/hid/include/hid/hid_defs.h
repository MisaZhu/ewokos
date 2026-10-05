/*
 * hid_defs.h: transport-independent HID constants.
 *
 * Everything a HID host needs that is a property of the HID
 * specification itself rather than of the bus it arrived on: the report
 * IDs used by the subscriber fan-out, the fixed event sizes consumers
 * read back, the boot-protocol report layouts and the usage pages/usages
 * the report-descriptor parser matches on.
 *
 * Shared by usbhostd (HID over USB) and btd (HIDP over classic
 * Bluetooth, HOGP over GATT) so a report decoded by either produces the
 * identical bytes for hid_keybd/hid_moused/hid_touchd.
 */
#ifndef __HID_DEFS_H__
#define __HID_DEFS_H__

#include <stdint.h>

/* subscriber fan-out: report IDs selected with fcntl(cmd 0), and the
   fixed event sizes read back from the device node */
#define HID_REPORT_ID_MOUSE 1u
#define HID_REPORT_ID_KEYBOARD 2u
#define HID_REPORT_ID_TOUCH 3u

#define HID_QUEUE_DEPTH 32
#define HID_MAX_EVENT_SIZE 8
#define HID_POINTER_EVENT_SIZE 7
#define HID_KEYBOARD_EVENT_SIZE 8
#define HID_MAX_REPORT 64
#define HID_MAX_USAGE_LIST 32

/* boot-protocol keyboard report: [modifiers, reserved, key1..key6] */
#define HID_KEYBOARD_REPORT_SIZE 8
#define HID_KEYBOARD_FIRST_KEY_IDX 2
#define HID_KEYBOARD_MAX_KEYS 6

/* boot-protocol mouse report: [buttons, dx, dy, (wheel)] */
#define HID_MOUSE_REPORT_SIZE 3
#define HID_MOUSE_BUTTON_LEFT (1u << 0)
#define HID_MOUSE_BUTTON_RIGHT (1u << 1)
#define HID_MOUSE_BUTTON_MIDDLE (1u << 2)

/* HID usage pages and usages referenced by the report parsers */
#define HID_USAGE_PAGE_GENERIC_DESKTOP 0x01
#define HID_USAGE_PAGE_BUTTON 0x09
#define HID_USAGE_PAGE_DIGITIZER 0x0D
#define HID_USAGE_POINTER 0x01
#define HID_USAGE_MOUSE 0x02
#define HID_USAGE_JOYSTICK 0x04
#define HID_USAGE_GAMEPAD 0x05
#define HID_USAGE_KEYBOARD 0x06
#define HID_USAGE_TOUCH_SCREEN 0x04
#define HID_USAGE_TOUCH_PAD 0x05
#define HID_USAGE_FINGER 0x22
#define HID_USAGE_TIP_SWITCH 0x42
#define HID_USAGE_X 0x30
#define HID_USAGE_Y 0x31
#define HID_USAGE_WHEEL 0x38

#endif /* __HID_DEFS_H__ */
