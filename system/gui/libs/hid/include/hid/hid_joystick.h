/*
 * hid_joystick.h: normalized gamepad state served by hid_joystickd.
 *
 * hid_joystickd subscribes to HID_REPORT_ID_JOYSTICK on the HID host node
 * (/dev/hid0 over USB, /dev/bt0 over Bluetooth), which carries the RAW
 * gamepad report prefix, decodes the device-specific layout (DualShock 4,
 * DualSense, Xbox GIP) and re-publishes one fixed js_evt_t per state change
 * on its own char device (/dev/js0, or /dev/js1 for the BT instance).
 *
 * This header is the producer/consumer contract: a consumer opens the node
 * and reads sizeof(js_evt_t) bytes per event. Axes are full-range signed
 * (centered at 0), triggers are 0..65535, buttons are the JS_BTN_* bitmask
 * and the d-pad is a 0..7 compass hat (JS_DPAD_NONE when released).
 */
#ifndef __HID_JOYSTICK_H__
#define __HID_JOYSTICK_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* face buttons use the physical Xbox/PlayStation positions, so a caller can
   map by intent regardless of the source pad's own labels */
#define JS_BTN_A      (1u << 0)   /* Xbox A / PS Cross   */
#define JS_BTN_B      (1u << 1)   /* Xbox B / PS Circle  */
#define JS_BTN_X      (1u << 2)   /* Xbox X / PS Square  */
#define JS_BTN_Y      (1u << 3)   /* Xbox Y / PS Triangle*/
#define JS_BTN_LB     (1u << 4)   /* left bumper  / L1   */
#define JS_BTN_RB     (1u << 5)   /* right bumper / R1   */
#define JS_BTN_LT     (1u << 6)   /* left trigger digital  / L2 */
#define JS_BTN_RT     (1u << 7)   /* right trigger digital / R2 */
#define JS_BTN_SELECT (1u << 8)   /* Back / View / Share / Create */
#define JS_BTN_START  (1u << 9)   /* Start / Menu / Options */
#define JS_BTN_LS     (1u << 10)  /* left stick click  / L3 */
#define JS_BTN_RS     (1u << 11)  /* right stick click / R3 */
#define JS_BTN_HOME   (1u << 12)  /* PS button / Xbox Guide */
#define JS_BTN_TOUCH  (1u << 13)  /* touchpad click (PS) */
#define JS_BTN_MUTE   (1u << 14)  /* mic mute (DualSense) */

/* d-pad hat: compass positions, or JS_DPAD_NONE when centered/released */
enum {
    JS_DPAD_N = 0,
    JS_DPAD_NE,
    JS_DPAD_E,
    JS_DPAD_SE,
    JS_DPAD_S,
    JS_DPAD_SW,
    JS_DPAD_W,
    JS_DPAD_NW,
    JS_DPAD_NONE = 8
};

typedef struct {
    uint32_t buttons;      /* JS_BTN_* bitmask */
    int16_t  lx, ly;       /* left stick,  -32768..32767 */
    int16_t  rx, ry;       /* right stick, -32768..32767 */
    uint16_t lt, rt;       /* analog triggers, 0..65535 */
    uint8_t  dpad;         /* JS_DPAD_* hat */
    uint8_t  connected;    /* 1 once a report has been decoded */
    uint8_t  reserved[2];
} __attribute__((packed)) js_evt_t;

#ifdef __cplusplus
}
#endif

#endif /* __HID_JOYSTICK_H__ */
