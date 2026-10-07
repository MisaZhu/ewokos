/* btd_hid.c - HIDP, HID channel state and /dev/bt0 subscriber
          fan-out.
   Carved out of the former monolithic btd.c; shared types,
   constants and cross-module declarations live in btd_int.h. */
#include "btd_int.h"

bt_hid_chan_t _hid;

uint64_t _sub_reassert_ms = 0;

static bool bt_hid_chan_up(void) {
    return _hid.active && _hid.ctrl != NULL && _hid.intr != NULL &&
        _hid.ctrl->state == L2CAP_STATE_OPEN && _hid.intr->state == L2CAP_STATE_OPEN;
}

void bt_hid_check_up(void) {
    char addr[24];

    if (!_hid.active || _hid.up || !bt_hid_chan_up()) {
        return;
    }
    _hid.up = true;
    /* HID setup is complete only after BOTH L2CAP channels are configured.
       Sending SET_PROTOCOL on control alone races the peer's interrupt setup.
       A gamepad has no boot protocol (it is defined only for keyboards and
       mice) and always reports in report protocol, so skip the request and
       let its full frames flow straight to bt_hid_handle_report. */
    if (!_hid.is_gamepad) {
        uint8_t hidp = HIDP_TRANS_SET_PROTOCOL | HIDP_PROTOCOL_BOOT;
        _hid.boot_protocol_ok = false;
        _hid.boot_protocol_pending = l2cap_send_pdu(_hid.acl_handle,
            _hid.ctrl->remote_cid, &hidp, 1) == 0;
        if (!_hid.boot_protocol_pending) {
            slog("bluetooth hid set_protocol_send_failed h=0x%04x\n", _hid.acl_handle);
        }
    }
    bt_addr_to_str(_hid.addr, addr, sizeof(addr));
    bt_emit("hid_ok %s handle=0x%04X\n", addr, _hid.acl_handle);
}

/* both channels of one link are gone: forget the HID session */
void bt_hid_link_closed(uint16_t handle, const char* reason) {
    char addr[24];
    bool was_up;

    if (!_hid.active || _hid.acl_handle != handle) {
        return;
    }
    was_up = _hid.up;
    bt_addr_to_str(_hid.addr, addr, sizeof(addr));
    memset(&_hid, 0, sizeof(_hid));
    if (was_up) {
        bt_emit("hid_disconnect %s reason=%s\n", addr, reason);
    }
}

void bt_hid_stop(void) {
    if (!_hid.active) {
        return;
    }
    /* HID disconnect order is interrupt first, then control. */
    if (_hid.intr != NULL) {
        l2cap_chan_close(_hid.intr, true);
    }
    if (_hid.ctrl != NULL) {
        l2cap_chan_close(_hid.ctrl, true);
    }
    bt_hid_link_closed(_hid.acl_handle, "closed");
}

/* A peer that never opens interrupt must not leave an active half-session
   blocking attachment forever. Repeated start requests do not extend this wait. */
void bt_hid_step(void) {
    if (_hid.active && _hid.ctrl != NULL && _hid.ctrl->incoming &&
            _hid.ctrl->state == L2CAP_STATE_OPEN && _hid.intr == NULL) {
        uint64_t now = kernel_tic_ms(0);
        if (_hid.intr_wait_ms == 0) {
            _hid.intr_wait_ms = now +
                L2CAP_STEP_TIMEOUT_MS * (L2CAP_STEP_MAX_RETRIES + 1);
        }
        else if (now >= _hid.intr_wait_ms) {
            slog("bluetooth hid peer_intr_timeout h=0x%04x\n", _hid.acl_handle);
            bt_hid_stop();
        }
    }
    else {
        _hid.intr_wait_ms = 0;
    }
    bt_hid_check_up();
}

/* ---------------- HIDP ---------------- */

/* Cursor gain for the combo touchpad's report-protocol pointer axes (see
   bt_hid_handle_report). Its raw 12-bit deltas are much smaller than a mouse's
   per-report counts, so they are multiplied by this before the int8 clamp.
   3 roughly matches a normal mouse; raise for faster, lower for finer. */
#define BT_TOUCHPAD_GAIN 3

/* Fan a relative pointer report whose scaled deltas may be wider than the
   single signed byte the 7-byte pointer event carries. Clamping to that byte
   (what a plain bt_hid_dispatch_mouse does) silently drops the excess, so a
   large high-resolution touchpad moves the cursor LESS than the finger every
   report and feels both slow and laggy. Emit the full delta as a short run of
   <=127 events instead; hid_moused coalesces consecutive moves inside its
   flush window, so the run collapses back to one cursor displacement equal to
   the full delta. The run length is bounded to stay well clear of the 32-deep
   subscriber queue even for a maximum-magnitude report. */
#define BT_MOUSE_MAX_SPLIT 8
static void bt_hid_dispatch_mouse_rel(uint8_t btn, int32_t dx, int32_t dy,
        int8_t wheel) {
    int n = 0;

    while (n < BT_MOUSE_MAX_SPLIT) {
        int32_t cx = dx > 127 ? 127 : (dx < -127 ? -127 : dx);
        int32_t cy = dy > 127 ? 127 : (dy < -127 ? -127 : dy);
        uint8_t evt[HID_MAX_EVENT_SIZE];

        memset(evt, 0, sizeof(evt));
        evt[0] = btn;
        evt[1] = (uint8_t)cx;
        evt[2] = (uint8_t)cy;
        evt[3] = (uint8_t)(n == 0 ? wheel : 0);
        bt_hid_dispatch_mouse(evt);
        dx -= cx;
        dy -= cy;
        n++;
        if (dx == 0 && dy == 0) {
            break;
        }
    }
}

/* Route one report from the classic interrupt channel.

   A boot-protocol report is self-identifying by length: a keyboard always
   sends the 8-byte [modifiers, reserved, key1..key6] layout, a boot mouse the
   3- or 4-byte [buttons, dx, dy, (wheel)] one. But SET_PROTOCOL BOOT is
   per-interface, and a combo peripheral (folding keyboard + touchpad) forces
   only its keyboard into boot while the touchpad keeps REPORT protocol: it
   then sends a longer frame carrying a leading Report ID. Such a frame must
   NOT go through the boot length heuristic - the Report ID would be misread
   as the button byte (0x07 = all three buttons stuck) and the axes would be
   off by one, which is exactly a touchpad that "moves wrong". */
void bt_hid_handle_report(const uint8_t* data, size_t len) {
    uint8_t evt[HID_MAX_EVENT_SIZE];

    if (len < 1) {
        return;
    }
    /*
     * Gamepad: a CoD-flagged pad, or a full report-protocol frame matching a
     * known PlayStation input report even when the pad reconnected with a
     * zero CoD (bond store): the expanded Bluetooth reports (0x11 DualShock
     * 4, 0x31 DualSense) are far longer than a boot frame, and the default
     * truncated report is exactly 10 bytes with Report ID 0x01. Without this
     * guard the >=8 length heuristic below would misread the stick bytes as
     * keycodes. Forward the raw prefix to the joystick subscribers and let
     * hid_joystickd decode the device-specific layout. The match latches
     * is_gamepad so every later frame routes correctly.
     */
    if (_hid.is_gamepad ||
            (len > HID_KEYBOARD_REPORT_SIZE &&
             (data[0] == 0x11 || data[0] == 0x31)) ||
            (len == 10 && data[0] == 0x01)) {
        _hid.is_gamepad = true;
        bt_hid_dispatch_joystick(data, len);
        return;
    }

    if (len >= HID_KEYBOARD_REPORT_SIZE) {
        memset(evt, 0, sizeof(evt));
        memcpy(evt, data, HID_KEYBOARD_REPORT_SIZE);
        bt_hid_dispatch_keyboard(evt);
        return;
    }
    /* Report-protocol pointer with a leading Report ID. This combo touchpad
       packs two signed 12-bit RELATIVE axes into three bytes:
         data[2]       = X[7:0]
         data[3] low   = X[11:8]  (sign extension of X)
         data[3] high  = Y[3:0]
         data[4]       = Y[11:4]
       Both axes are relative deltas. Reading data[4] alone as dy only grabs
       Y's top 8 bits (Y>>4), which is why Y crawled. A plain report-protocol
       mouse is [id][btn][dx][dy][wheel] (len 5), both axes int8 relatives. */
    if (len >= 5) {
        if (len >= 7) {
            int32_t x = (int32_t)((((uint32_t)data[3] & 0x0f) << 8) | data[2]);
            int32_t y = (int32_t)((((uint32_t)data[4]) << 4) |
                    (((uint32_t)data[3] & 0xf0) >> 4));
            if (x & 0x800) {
                x -= 0x1000;
            }
            if (y & 0x800) {
                y -= 0x1000;
            }
            /* A touchpad's finger resolution is far coarser than a mouse's DPI:
               its raw 12-bit deltas (only a few tens per report) drive the
               cursor too slowly when passed 1:1, so scale both axes by a fixed
               gain. On a LARGE touchpad the scaled delta of an ordinary swipe
               routinely exceeds the single signed byte the pointer event
               carries; clamping it here (the old behaviour) threw away the
               excess every report, so the cursor travelled less than the finger
               and the pad felt slow and laggy. bt_hid_dispatch_mouse_rel emits
               the whole distance as a short run of <=127 events that hid_moused
               coalesces back into one move. Raise/lower the gain to taste. */
            x *= BT_TOUCHPAD_GAIN;
            y *= BT_TOUCHPAD_GAIN;
            bt_hid_dispatch_mouse_rel(data[1], x, y, (int8_t)data[6]);
        }
        else {
            memset(evt, 0, sizeof(evt));
            evt[0] = data[1]; /* buttons (data[0] is the Report ID) */
            evt[1] = data[2]; /* dx (signed) */
            evt[2] = data[3]; /* dy (signed) */
            evt[3] = data[4]; /* wheel */
            bt_hid_dispatch_mouse(evt);
        }
        return;
    }
    if (len < 3) {
        return;
    }
    memset(evt, 0, sizeof(evt));
    evt[0] = data[0]; /* buttons: bit0 left, bit1 right, bit2 middle */
    evt[1] = data[1]; /* dx (signed) */
    evt[2] = data[2]; /* dy (signed) */
    if (len >= 4) {
        evt[3] = data[3]; /* wheel on report-protocol mice */
    }
    bt_hid_dispatch_mouse(evt);
}

void bt_hid_handle_ctrl(l2cap_chan_t* ch, const uint8_t* data, size_t len) {
    char addr[24];

    if (len < 1) {
        return;
    }
    switch (data[0] & 0xF0) {
    case HIDP_TRANS_HANDSHAKE:
        if (!_hid.active || _hid.ctrl != ch || !_hid.boot_protocol_pending) {
            break;
        }
        _hid.boot_protocol_pending = false;
        if (data[0] == HIDP_HANDSHAKE_SUCCESS) {
            _hid.boot_protocol_ok = true;
        }
        else {
            /* not a boot device or busy: reports keep arriving in
               report protocol, which for a plain mouse has the same
               [btn, dx, dy, (wheel)] layout */
            slog("bluetooth hid set_protocol refused=0x%02x\n", data[0] & 0x0f);
        }
        break;
    case HIDP_TRANS_HID_CONTROL:
        if (data[0] == HIDP_HID_CONTROL_VC_UNPLUG) {
            /* the mouse wants its virtual cable unplugged: close the
               channels and the ACL link, the pairing itself survives */
            bt_addr_to_str(_hid.addr, addr, sizeof(addr));
            bt_hid_stop();
            if (ch->acl_handle != 0) {
                bt_hci_disconnect(ch->acl_handle);
            }
            bt_emit("hid_unplug %s\n", addr);
        }
        break;
    default:
        break;
    }
}

/* ---------------- /dev/bt0 subscriber fan-out ----------------
   the queue, the report-id selection and the directed wakes all come from
   libhid, shared verbatim with usbhostd's /dev/hid0. What stays here is
   btd's own twist: report id 0 is not a HID report but the daemon's
   command/event text stream, so open/close/fcntl go straight to libhid
   while read() and check_poll_events() multiplex. */

/* fan one pointer event out; libhid wakes each subscriber only on its
   queue's empty -> non-empty edge (directed proc_wakeup_by). That edge can
   be spent on a generic IPC wait, so arm the bounded re-assert bt_loop
   runs while hid_backlog() still holds. */
void bt_hid_dispatch_mouse(const uint8_t* evt) {
    if (hid_dispatch_evt(HID_REPORT_ID_MOUSE, evt, HID_POINTER_EVENT_SIZE)) {
        _sub_reassert_ms = kernel_tic_ms(0) + BT_HID_REASSERT_MS;
    }
}

void bt_hid_dispatch_keyboard(const uint8_t* evt) {
    if (hid_dispatch_evt(HID_REPORT_ID_KEYBOARD, evt, HID_KEYBOARD_EVENT_SIZE)) {
        _sub_reassert_ms = kernel_tic_ms(0) + BT_HID_REASSERT_MS;
    }
}

/* Forward the raw gamepad report to the joystick subscribers. The prefix is
   truncated/zero-padded to the fixed HID_JOYSTICK_RAW_SIZE so every queued
   event has an identical stride and hid_joystickd can frame its drain loop;
   all the input fields of the DualShock 4/5 and Xbox GIP reports live inside
   that window. */
void bt_hid_dispatch_joystick(const uint8_t* raw, size_t len) {
    uint8_t evt[HID_JOYSTICK_RAW_SIZE];

    memset(evt, 0, sizeof(evt));
    if (len > sizeof(evt)) {
        len = sizeof(evt);
    }
    memcpy(evt, raw, len);
    if (hid_dispatch_evt(HID_REPORT_ID_JOYSTICK, evt, HID_JOYSTICK_RAW_SIZE)) {
        _sub_reassert_ms = kernel_tic_ms(0) + BT_HID_REASSERT_MS;
    }
}

static void bt_hid_session_init(uint16_t handle, const uint8_t* addr) {
    if (handle == 0) {
        return;
    }
    /* a stale session on a dead handle must not leak its channels */
    if (_hid.active && _hid.acl_handle != handle) {
        bt_hid_stop();
    }
    if (!_hid.active) {
        memset(&_hid, 0, sizeof(_hid));
        _hid.active = true;
        _hid.acl_handle = handle;
        if (addr != NULL) {
            memcpy(_hid.addr, addr, 6);
        }
    }
    /* A classic peripheral paged straight from the bond store never went
       through an inquiry, so its Class-of-Device is still zero and xbt's
       type column (derived from the CoD major class) reads Unknown. Once
       the link really carries HID, record a Peripheral-major CoD. */
    {
        bt_device_t* d = bt_find_device_by_handle(handle);
        if (d != NULL && d->class_of_device == 0) {
            d->class_of_device = 0x002500;
        }
        /* A joystick/gamepad CoD marks a full report-protocol pad: flag the
           session so bt_hid_handle_report forwards its raw frames to the
           joystick subscribers instead of the boot keyboard/mouse heuristic,
           and bt_hid_check_up skips the boot-protocol SET_PROTOCOL. */
        if (d != NULL) {
            _hid.is_gamepad = bt_cod_is_gamepad(d->class_of_device);
        }
    }
}

/* Accepting a peer's channel must not originate the other half of the pair. */
void bt_hid_accept(l2cap_chan_t* ch) {
    bt_device_t* dev = bt_find_device_by_handle(ch->acl_handle);
    bt_hid_session_init(ch->acl_handle, dev != NULL ? dev->addr : NULL);
    if (ch->psm == L2CAP_PSM_HID_CTRL) {
        _hid.ctrl = ch;
    }
    else {
        _hid.intr = ch;
    }
}

void bt_hid_start(uint16_t handle, const uint8_t* addr) {
    if (handle == 0) {
        return;
    }
    /* Never bind classic HIDP to an LE handle. An LE peripheral is served by
       HOGP over GATT; classic L2CAP channels cannot ride an LE link, and if
       _hid.acl_handle were aliased to a live LE handle the duplicate-transport
       suppression at the end of bt_le_connect would disconnect the very HOGP
       link it is meant to protect (a dual-mode gamepad drops right after READY
       and loops). A dual-mode pad reached over LE is refused here; one truly
       on a classic ACL is not an LE-session handle and proceeds normally. */
    if (le_session_by_handle(handle) >= 0) {
        return;
    }
    bt_hid_session_init(handle, addr);
    if (_hid.ctrl == NULL || _hid.ctrl->state != L2CAP_STATE_OPEN) {
        _hid.ctrl = l2cap_chan_open(handle, L2CAP_PSM_HID_CTRL);
    }
    /* HID 5.2.2: the control-channel initiator also initiates interrupt.
       A peer-initiated reconnect waits for the peer, even on repeated starts
       from an attach poll or security callback. BlueZ follows the same split. */
    if (_hid.ctrl != NULL && _hid.ctrl->state == L2CAP_STATE_OPEN &&
            !_hid.ctrl->incoming &&
            (_hid.intr == NULL || _hid.intr->state != L2CAP_STATE_OPEN)) {
        _hid.intr = l2cap_chan_open(handle, L2CAP_PSM_HID_INTR);
    }
}

/* drop every L2CAP channel (the ACL links die with the radio anyway) */
void bt_hid_stack_reset(void) {
    memset(&_l2chans, 0, sizeof(_l2chans));
    memset(&_hid, 0, sizeof(_hid));
    l2cap_rx_reset();
    _acl_credits = 0;
    bt_le_stack_reset();
}
