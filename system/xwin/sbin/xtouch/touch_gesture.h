#ifndef TOUCH_GESTURE_H
#define TOUCH_GESTURE_H

#include <stddef.h>
#include <stdint.h>

/* Gesture state machine over multi-contact touch frames.
 *
 * Ported from the q8b Goodix Berlin driver: event frames carry exactly the
 * currently active contacts, so a contact that stops appearing has been
 * lifted. The caller feeds one frame per touch event and a periodic tick
 * for timeouts; recognized gestures are returned through the emit callback
 * as button/scroll events in the source device's coordinates.
 *
 * xtouch runs it in -m (touchpad) mode for the two-finger gestures only:
 * frames are fed while two or more fingers are down, plus one final frame
 * when they lift so the right-click decision runs; single-finger click /
 * drag / move stays with the precision-touchpad state machine. */

#define GESTURE_MAX_TOUCH 10U

/* Single finger is held this long before LEFT down is committed (a second
 * finger landing inside the window turns the press into a two-finger
 * gesture instead of a left click).
 *
 * This window must comfortably exceed the landing skew between the two
 * fingers of a two-finger tap/swipe: they rarely land in the same panel
 * frame, and if the first finger commits LEFT_DOWN (hold) before the second
 * arrives, the gesture emits a phantom left click -- and jumps the cursor
 * for a swipe -- ahead of the right click / scroll.  250 ms covers normal
 * finger landing skew; a quick lift is still a click and movement past the
 * tap radius still commits a drag immediately, so single-finger tap/drag
 * latency is unchanged (only stationary hold-to-drag waits a bit longer). */
#define GESTURE_PENDING_MS 250U

/* Source-device pixels.  A single finger past this commits a drag; a
 * two-finger midpoint displaced more than this cancels the right click. */
#define GESTURE_TAP_MOVE_PX 50U

/* Midpoint travel that produces one wheel notch.  Wheel events carry no
 * magnitude, so a swipe is quantized into notches at this granularity. */
#define GESTURE_SCROLL_NOTCH_PX 25U

typedef enum {
    GESTURE_EVT_NONE = 0,
    GESTURE_EVT_LEFT_DOWN,
    GESTURE_EVT_LEFT_UP,
    GESTURE_EVT_RIGHT_DOWN,
    GESTURE_EVT_RIGHT_UP,
    GESTURE_EVT_SCROLL_UP,
    GESTURE_EVT_SCROLL_DOWN,
} gesture_evt_t;

typedef struct {
    uint8_t id; /* touch id, must be < GESTURE_MAX_TOUCH */
    uint16_t x;
    uint16_t y;
} gesture_contact_t;

typedef struct {
    uint8_t count;
    gesture_contact_t contacts[GESTURE_MAX_TOUCH];
} gesture_frame_t;

typedef void (*gesture_emit_fn)(gesture_evt_t evt, uint16_t x, uint16_t y);

void gesture_reset(void);
void gesture_process(const gesture_frame_t* frame, uint64_t now_ms,
        gesture_emit_fn emit);
void gesture_tick(uint64_t now_ms, gesture_emit_fn emit);

#endif