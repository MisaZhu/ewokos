#include <stdbool.h>

#include "touch_gesture.h"

/* States:
 *
 *   GS_IDLE    nothing down
 *   GS_PENDING one finger down, LEFT not yet committed: a quick lift is a
 *              click, movement past the tap radius commits a drag, and a
 *              second finger re-routes the press into a two-finger gesture
 *   GS_LEFT    one finger down, LEFT committed (drag / hold)
 *   GS_TWO     two fingers down: midpoint movement emits scroll notches; a
 *              release without scrolling and near the start point is a
 *              right click
 *   GS_REST    a two-finger gesture ended with a finger still down; the
 *              leftover contact is suppressed until every finger lifts so
 *              staggered finger lifts never produce a phantom click */
enum {
    GS_IDLE = 0,
    GS_PENDING,
    GS_LEFT,
    GS_TWO,
    GS_REST,
};

static unsigned _state = GS_IDLE;
static uint16_t _px, _py;          /* tracked contact position */
static uint16_t _down_x, _down_y;  /* pending press start position */
static uint64_t _pending_ms;
static uint8_t _two_ids[2];        /* contacts driving the two-finger gesture */
static uint16_t _two_mid_x, _two_mid_y;   /* current midpoint */
static uint16_t _two_prev_y;       /* midpoint y of the previous frame */
static uint16_t _two_start_x, _two_start_y; /* midpoint at gesture start */
static int32_t _scroll_acc;        /* undrained midpoint travel, + is down */
static bool _two_scrolled;

void gesture_reset(void) {
    _state = GS_IDLE;
}

/* Same gesture, different contact pair (a finger switched): keep the
 * scroll/click state so a lift after a finger walk cannot re-arm a
 * phantom right click. */
static void two_reanchor(const gesture_contact_t* a, const gesture_contact_t* b) {
    _two_ids[0] = a->id;
    _two_ids[1] = b->id;
    _two_mid_x = (uint16_t)(((uint32_t)a->x + b->x) / 2U);
    _two_mid_y = (uint16_t)(((uint32_t)a->y + b->y) / 2U);
    _two_prev_y = _two_mid_y;
}

/* Fresh two-finger gesture: full reset of the scroll/click state. */
static void two_anchor(const gesture_contact_t* a, const gesture_contact_t* b) {
    two_reanchor(a, b);
    _two_start_x = _two_mid_x;
    _two_start_y = _two_mid_y;
    _scroll_acc = 0;
    _two_scrolled = false;
}

/* Keep tracking the original contact pair when both are still present so a
 * frame-order shuffle cannot jump the midpoint; otherwise re-anchor on the
 * frame's first two contacts and suppress this frame's delta.  Returns
 * false when re-anchored. */
static bool two_pair(const gesture_frame_t* frame,
        const gesture_contact_t** a, const gesture_contact_t** b) {
    const gesture_contact_t* found_a = NULL;
    const gesture_contact_t* found_b = NULL;

    for (unsigned i = 0; i < frame->count; i++) {
        if (found_a == NULL && frame->contacts[i].id == _two_ids[0])
            found_a = &frame->contacts[i];
        else if (found_b == NULL && frame->contacts[i].id == _two_ids[1])
            found_b = &frame->contacts[i];
    }
    if (found_a != NULL && found_b != NULL) {
        *a = found_a;
        *b = found_b;
        return true;
    }
    two_reanchor(&frame->contacts[0], &frame->contacts[1]);
    return false;
}

/* Drain the accumulated midpoint travel into direction-only notches: wheel
 * events carry no magnitude, so a swipe is quantized at a fixed pixel
 * step.  Fingers moving down scroll the content down. */
static void two_drain(gesture_emit_fn emit) {
    while (_scroll_acc <= -(int32_t)GESTURE_SCROLL_NOTCH_PX) {
        emit(GESTURE_EVT_SCROLL_UP, _two_mid_x, _two_mid_y);
        _scroll_acc += (int32_t)GESTURE_SCROLL_NOTCH_PX;
        _two_scrolled = true;
    }
    while (_scroll_acc >= (int32_t)GESTURE_SCROLL_NOTCH_PX) {
        emit(GESTURE_EVT_SCROLL_DOWN, _two_mid_x, _two_mid_y);
        _scroll_acc -= (int32_t)GESTURE_SCROLL_NOTCH_PX;
        _two_scrolled = true;
    }
}

void gesture_process(const gesture_frame_t* frame, uint64_t now_ms,
        gesture_emit_fn emit) {
    unsigned n = frame->count;
    if (n > GESTURE_MAX_TOUCH)
        n = GESTURE_MAX_TOUCH;

    switch (_state) {
    case GS_IDLE:
        if (n == 1) {
            _state = GS_PENDING;
            _down_x = _px = frame->contacts[0].x;
            _down_y = _py = frame->contacts[0].y;
            _pending_ms = now_ms;
        } else if (n >= 2) {
            two_anchor(&frame->contacts[0], &frame->contacts[1]);
            _state = GS_TWO;
        }
        break;

    case GS_PENDING:
        if (n >= 2) {
            two_anchor(&frame->contacts[0], &frame->contacts[1]);
            _state = GS_TWO;
        } else if (n == 1) {
            int32_t dx, dy;
            _px = frame->contacts[0].x;
            _py = frame->contacts[0].y;
            dx = (int32_t)_px - _down_x;
            dy = (int32_t)_py - _down_y;
            if (dx * dx + dy * dy >
                    (int32_t)GESTURE_TAP_MOVE_PX *
                    (int32_t)GESTURE_TAP_MOVE_PX) {
                emit(GESTURE_EVT_LEFT_DOWN, _px, _py);
                _state = GS_LEFT;
            }
        } else {
            /* Released while still deciding: a tap is a click. */
            emit(GESTURE_EVT_LEFT_DOWN, _down_x, _down_y);
            emit(GESTURE_EVT_LEFT_UP, _px, _py);
            _state = GS_IDLE;
        }
        break;

    case GS_LEFT:
        if (n >= 2) {
            emit(GESTURE_EVT_LEFT_UP, _px, _py);
            two_anchor(&frame->contacts[0], &frame->contacts[1]);
            _state = GS_TWO;
        } else if (n == 1) {
            _px = frame->contacts[0].x;
            _py = frame->contacts[0].y;
            emit(GESTURE_EVT_LEFT_DOWN, _px, _py);
        } else {
            emit(GESTURE_EVT_LEFT_UP, _px, _py);
            _state = GS_IDLE;
        }
        break;

    case GS_TWO:
        if (n >= 2) {
            const gesture_contact_t *a, *b;
            if (two_pair(frame, &a, &b)) {
                /* Both original fingers present: accumulate the midpoint
                 * travel of this frame, then drain it into notches. */
                _two_mid_x = (uint16_t)(((uint32_t)a->x + b->x) / 2U);
                _two_mid_y = (uint16_t)(((uint32_t)a->y + b->y) / 2U);
                _scroll_acc += (int32_t)_two_mid_y - _two_prev_y;
                _two_prev_y = _two_mid_y;
                two_drain(emit);
            }
        } else {
            int32_t dx = (int32_t)_two_mid_x - _two_start_x;
            int32_t dy = (int32_t)_two_mid_y - _two_start_y;
            /* A still press released without scrolling is a right click;
             * a scroll already reported its events and must not click. */
            if (!_two_scrolled &&
                    dx * dx + dy * dy <=
                    (int32_t)GESTURE_TAP_MOVE_PX *
                    (int32_t)GESTURE_TAP_MOVE_PX) {
                emit(GESTURE_EVT_RIGHT_DOWN, _two_mid_x, _two_mid_y);
                emit(GESTURE_EVT_RIGHT_UP, _two_mid_x, _two_mid_y);
            }
            _state = (n == 0) ? GS_IDLE : GS_REST;
        }
        break;

    case GS_REST:
        if (n == 0) {
            _state = GS_IDLE;
        } else if (n >= 2) {
            two_anchor(&frame->contacts[0], &frame->contacts[1]);
            _state = GS_TWO;
        }
        break;
    }
}

void gesture_tick(uint64_t now_ms, gesture_emit_fn emit) {
    if (_state == GS_PENDING &&
            now_ms - _pending_ms >= GESTURE_PENDING_MS) {
        emit(GESTURE_EVT_LEFT_DOWN, _px, _py);
        _state = GS_LEFT;
    }
}