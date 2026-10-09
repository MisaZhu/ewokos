/* btd_hid.c - HIDP, HID channel state and /dev/bt0 subscriber
          fan-out.
   Carved out of the former monolithic btd.c; shared types,
   constants and cross-module declarations live in btd_int.h. */
#include "btd_int.h"

bt_hid_chan_t _hids[MAX_CLASSIC_HID_SESSIONS];

uint64_t _sub_reassert_ms = 0;

/* The live session owning this ACL link, NULL when the link has none. */
bt_hid_chan_t* bt_hid_by_handle(uint16_t handle) {
    int i;

    if (handle == 0) {
        return NULL;
    }
    for (i = 0; i < MAX_CLASSIC_HID_SESSIONS; ++i) {
        if (_hids[i].active && _hids[i].acl_handle == handle) {
            return &_hids[i];
        }
    }
    return NULL;
}

/* A slot available for a new session, NULL when both are taken. */
bt_hid_chan_t* bt_hid_slot_free(void) {
    int i;

    for (i = 0; i < MAX_CLASSIC_HID_SESSIONS; ++i) {
        if (!_hids[i].active) {
            return &_hids[i];
        }
    }
    return NULL;
}

static bool bt_hid_chan_up(const bt_hid_chan_t* h) {
    return h->active && h->ctrl != NULL && h->intr != NULL &&
        h->ctrl->state == L2CAP_STATE_OPEN && h->intr->state == L2CAP_STATE_OPEN;
}

void bt_hid_check_up(bt_hid_chan_t* h) {
    char addr[24];

    if (h == NULL || !h->active || h->up || !bt_hid_chan_up(h)) {
        return;
    }
    h->up = true;
    /* HID setup is complete only after BOTH L2CAP channels are configured.
       Sending SET_PROTOCOL on control alone races the peer's interrupt setup.
       Keyboards/mice get BOOT so their fixed-length frames parse by length;
       a gamepad gets REPORT instead: cheap pad firmware (observed on the
       Zikway "Xbox Wireless Controller" clone, whose own record says
       VirtualCable=FALSE) powers up in boot protocol and only starts
       streaming input after the host's SET_PROTOCOL(REPORT), the same
       request real hosts send while enumerating a BT HID device. */
    {
        uint8_t hidp = HIDP_TRANS_SET_PROTOCOL |
            (h->is_gamepad ? HIDP_PROTOCOL_REPORT : HIDP_PROTOCOL_BOOT);
        h->boot_protocol_ok = false;
        h->boot_protocol_pending = l2cap_send_pdu(h->acl_handle,
            h->ctrl->remote_cid, &hidp, 1) == 0;
        if (!h->boot_protocol_pending) {
            slog("bluetooth hid set_protocol_send_failed h=0x%04x\n", h->acl_handle);
        }
    }
    bt_addr_to_str(h->addr, addr, sizeof(addr));
    bt_emit("hid_ok %s handle=0x%04X\n", addr, h->acl_handle);
}

/* both channels of one link are gone: forget that HID session */
void bt_hid_link_closed(uint16_t handle, const char* reason) {
    char addr[24];
    bool was_up;
    bt_hid_chan_t* h = bt_hid_by_handle(handle);

    if (h == NULL) {
        return;
    }
    was_up = h->up;
    bt_addr_to_str(h->addr, addr, sizeof(addr));
    /* a key or button still down at the drop would otherwise stay down on
       the consumer side forever (hid_keybd auto-repeat through xim_none) */
    bt_hid_release_held(&h->held);
    memset(h, 0, sizeof(*h));
    if (was_up) {
        bt_emit("hid_disconnect %s reason=%s\n", addr, reason);
    }
}

void bt_hid_stop(bt_hid_chan_t* h) {
    if (h == NULL || !h->active) {
        return;
    }
    /* HID disconnect order is interrupt first, then control. */
    if (h->intr != NULL) {
        l2cap_chan_close(h->intr, true);
    }
    if (h->ctrl != NULL) {
        l2cap_chan_close(h->ctrl, true);
    }
    bt_hid_link_closed(h->acl_handle, "closed");
}

/* Upper bound for a gamepad's SDP record enumeration before its HID channels
   open anyway: generous against a slow fetch, short against a pad that
   ignores our SDP connection entirely. */
#define BT_HID_SDP_MAP_MS 3000

/* Head start a gamepad gets to open its own HID channels after enumeration
   before we open them outbound (bt_hid_start). The self-reconnecting pad
   does so within milliseconds; a host-waiting pad costs this much latency. */
#define BT_HID_PEER_OPEN_MS 3000

/* A peer that never opens interrupt must not leave an active half-session
   blocking attachment forever. Repeated start requests do not extend this wait. */
void bt_hid_step(void) {
    int i;

    for (i = 0; i < MAX_CLASSIC_HID_SESSIONS; ++i) {
        bt_hid_chan_t* h = &_hids[i];

        if (!h->active) {
            continue;
        }
        if (h->ctrl != NULL && h->ctrl->incoming &&
                h->ctrl->state == L2CAP_STATE_OPEN && h->intr == NULL) {
            uint64_t now = kernel_tic_ms(0);
            if (h->intr_wait_ms == 0) {
                h->intr_wait_ms = now +
                    L2CAP_STEP_TIMEOUT_MS * (L2CAP_STEP_MAX_RETRIES + 1);
            }
            else if (now >= h->intr_wait_ms) {
                slog("bluetooth hid peer_intr_timeout h=0x%04x\n", h->acl_handle);
                bt_hid_stop(h);
                continue;
            }
        }
        else {
            h->intr_wait_ms = 0;
        }
        /* Report silence does not establish LE support. Channel setup timeouts
           are handled by l2cap_step; never disconnect an open classic link or
           discard its bond merely because no input report has arrived. */
        bt_hid_check_up(h);
        /* Gamepad enumeration-first state machine: the record fetch normally
           completes well inside the deadline; a pad that ignores our SDP
           connection must not block HID setup longer than BT_HID_SDP_MAP_MS. */
        if (h->is_gamepad && !h->sdp_map_done) {
            uint64_t now = kernel_tic_ms(0);
            if (h->sdp_deadline_ms == 0) {
                h->sdp_deadline_ms = now + BT_HID_SDP_MAP_MS;
            }
            if (!h->sdp_map_started) {
                bt_sdp_client_kick(h);
            }
            if (now >= h->sdp_deadline_ms) {
                slog("bt sdp_cli_skip h=%04x\n", (unsigned)h->acl_handle);
                if (h->sdp != NULL) {
                    l2cap_chan_close(h->sdp, true);
                }
                h->sdp_map_done = true;
            }
        }
        /* Enumeration done and nothing in flight: (re)start the channels. */
        if (h->is_gamepad && h->sdp_map_done && !h->up && h->ctrl == NULL) {
            bt_hid_start(h->acl_handle, h->addr);
        }
    }
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
static void bt_hid_dispatch_mouse_rel(bt_hid_held_t* held, uint8_t btn,
        int32_t dx, int32_t dy, int8_t wheel) {
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
        bt_hid_dispatch_mouse(held, evt);
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
void bt_hid_handle_report(bt_hid_chan_t* h, bt_hid_held_t* held,
        const uint8_t* data, size_t len) {
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
     * is_gamepad so every later frame routes correctly. The LE fallback
     * caller passes h == NULL: it has no session to latch, and an unrelated
     * classic session must not misroute this report.
     */
    if ((h != NULL && h->is_gamepad) ||
            (len > HID_KEYBOARD_REPORT_SIZE &&
             (data[0] == 0x11 || data[0] == 0x31)) ||
            (len == 10 && data[0] == 0x01)) {
        if (h != NULL) {
            h->is_gamepad = true;
            /* Descriptor-driven layout from the pad's own SDP HID record
               (the Zikway "Xbox" clone streams a 17-byte report 0x01 that
               hid_joystickd would otherwise misread as a PlayStation
               truncated report). Normalize here and forward the js_evt_t
               under the reserved 0x05 tag, exactly like the HOGP path; the
               pad's non-input reports (battery 0x02, ...) are dropped. */
            if (h->joystick_ok) {
                if (h->joystick.has_report_id &&
                        data[0] != h->joystick.report_id) {
                    return;
                }
                js_evt_t je;
                uint8_t frame[1 + sizeof(js_evt_t)];
                if (joystick_normalize_report(&h->joystick, data, (int)len, &je) == 0) {
                    frame[0] = 0x05;
                    memcpy(frame + 1, &je, sizeof(js_evt_t));
                    bt_hid_dispatch_joystick(frame, sizeof(frame));
                    return;
                }
                /* length mismatch etc.: fall back to the raw-prefix path */
            }
        }
        bt_hid_dispatch_joystick(data, len);
        return;
    }

    if (len >= HID_KEYBOARD_REPORT_SIZE) {
        memset(evt, 0, sizeof(evt));
        memcpy(evt, data, HID_KEYBOARD_REPORT_SIZE);
        bt_hid_dispatch_keyboard(held, evt);
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
            bt_hid_dispatch_mouse_rel(held, data[1], x, y, (int8_t)data[6]);
        }
        else {
            memset(evt, 0, sizeof(evt));
            evt[0] = data[1]; /* buttons (data[0] is the Report ID) */
            evt[1] = data[2]; /* dx (signed) */
            evt[2] = data[3]; /* dy (signed) */
            evt[3] = data[4]; /* wheel */
            bt_hid_dispatch_mouse(held, evt);
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
    bt_hid_dispatch_mouse(held, evt);
}

void bt_hid_handle_ctrl(l2cap_chan_t* ch, const uint8_t* data, size_t len) {
    char addr[24];
    bt_hid_chan_t* h;

    if (len < 1) {
        return;
    }
    h = bt_hid_by_handle(ch->acl_handle);
    switch (data[0] & 0xF0) {
    case HIDP_TRANS_HANDSHAKE:
        if (h == NULL || h->ctrl != ch || !h->boot_protocol_pending) {
            break;
        }
        h->boot_protocol_pending = false;
        if (data[0] == HIDP_HANDSHAKE_SUCCESS) {
            h->boot_protocol_ok = true;
            if (h->is_gamepad) {
                /* the pad accepted REPORT protocol: input may start now.
                   Xbox One-style firmware (the Zikway clone advertises a PID
                   DC-Enable-Actuators output as its report ID 3) holds back
                   input reports until the host enables the actuators once.
                   Zero magnitudes: this wakes the stream without rumbling. */
                static const uint8_t ff_init[] = {
                    HIDP_DATA_OUTPUT, 0x03, 0x0F, 0, 0, 0, 0, 0, 0, 0
                };
                if (h->intr != NULL) {
                    (void)l2cap_send_pdu(h->acl_handle,
                        h->intr->remote_cid, ff_init, sizeof(ff_init));
                }
                /* Same enable as SET_REPORT on the control channel: the
                   classic equivalent of the GATT report write xpadneo-style
                   firmware actually honors, where the interrupt-channel
                   DATA|OUTPUT twin above is silently ignored. */
                static const uint8_t ff_init_sr[] = {
                    HIDP_TRANS_SET_REPORT | HIDP_REPORT_OUTPUT,
                    0x03, 0x0F, 0, 0, 0, 0, 0, 0, 0
                };
                (void)l2cap_send_pdu(h->acl_handle,
                    h->ctrl->remote_cid, ff_init_sr, sizeof(ff_init_sr));
                /* SET_IDLE 0 disables the pad's idle timer so its report
                   stream never parks; cheap firmware arms its input only
                   after this standard request. */
                static const uint8_t set_idle[] = { HIDP_TRANS_SET_IDLE, 0x00 };
                (void)l2cap_send_pdu(h->acl_handle,
                    h->ctrl->remote_cid, set_idle, sizeof(set_idle));
                /* GET_REPORT(Input 1) is mandatory in HIDP: poll-driven
                   firmware answers it on this channel, and streaming
                   firmware often starts streaming after the first pull. */
                static const uint8_t get_in[] = {
                    HIDP_TRANS_GET_REPORT | HIDP_REPORT_INPUT, 0x01
                };
                (void)l2cap_send_pdu(h->acl_handle,
                    h->ctrl->remote_cid, get_in, sizeof(get_in));
            }
        }
        else {
            /* not a boot device or busy: reports keep arriving in
               report protocol, which for a plain mouse has the same
               [btn, dx, dy, (wheel)] layout */
            slog("bluetooth hid set_protocol refused=0x%02x\n", data[0] & 0x0f);
        }
        break;
    case HIDP_TRANS_DATA:
        /* A GET_REPORT reply arrives on the control channel: strip the a1
           header and route it exactly like an interrupt-channel report. */
        if (h != NULL && data[0] == HIDP_DATA_INPUT && len > 1) {
            bt_hid_handle_report(h, &h->held, data + 1, len - 1);
        }
        break;
    case HIDP_TRANS_HID_CONTROL:
        if (data[0] == HIDP_HID_CONTROL_VC_UNPLUG && h != NULL) {
            /* the mouse wants its virtual cable unplugged: close the
               channels and the ACL link, the pairing itself survives */
            bt_addr_to_str(h->addr, addr, sizeof(addr));
            bt_hid_stop(h);
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
void bt_hid_dispatch_mouse(bt_hid_held_t* held, const uint8_t* evt) {
    if (held != NULL) {
        held->mouse_btn = evt[0] != 0;
    }
    if (hid_dispatch_evt(HID_REPORT_ID_MOUSE, evt, HID_POINTER_EVENT_SIZE)) {
        _sub_reassert_ms = kernel_tic_ms(0) + BT_HID_REASSERT_MS;
    }
}

void bt_hid_dispatch_keyboard(bt_hid_held_t* held, const uint8_t* evt) {
    if (held != NULL) {
        int i;

        held->kbd = false;
        for (i = 0; i < HID_KEYBOARD_REPORT_SIZE; ++i) {
            if (evt[i] != 0) {
                held->kbd = true;
                break;
            }
        }
    }
    if (hid_dispatch_evt(HID_REPORT_ID_KEYBOARD, evt, HID_KEYBOARD_EVENT_SIZE)) {
        _sub_reassert_ms = kernel_tic_ms(0) + BT_HID_REASSERT_MS;
    }
}

/* The peer dropped (out of range, battery, power switch) after a press and
   before its release report: nothing will ever clear that snapshot on the
   consumer side, so send it here. An all-zero keyboard snapshot is "no
   modifier, no key" and an all-zero pointer event is "no button, no motion",
   which is exactly what the peer would have sent. Idle links send nothing,
   so a disconnect never costs the subscribers a spurious wake. */
void bt_hid_release_held(bt_hid_held_t* held) {
    uint8_t evt[HID_MAX_EVENT_SIZE];

    if (held == NULL) {
        return;
    }
    memset(evt, 0, sizeof(evt));
    if (held->kbd) {
        bt_hid_dispatch_keyboard(held, evt);
    }
    if (held->mouse_btn) {
        bt_hid_dispatch_mouse(held, evt);
    }
    held->kbd = false;
    held->mouse_btn = false;
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

/* Find the session owning this link, or claim a free slot for it. Returns
   NULL only when both sessions are taken by other links; the L2CAP admission
   gate refuses channels before that can happen, so this is defensive. */
static bt_hid_chan_t* bt_hid_session_init(uint16_t handle, const uint8_t* addr) {
    bt_hid_chan_t* h;

    if (handle == 0) {
        return NULL;
    }
    h = bt_hid_by_handle(handle);
    if (h == NULL) {
        h = bt_hid_slot_free();
        if (h == NULL) {
            slog("bluetooth hid no_free_session h=0x%04x\n", handle);
            return NULL;
        }
        memset(h, 0, sizeof(*h));
        h->active = true;
        h->acl_handle = handle;
        if (addr != NULL) {
            memcpy(h->addr, addr, 6);
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
           bt_hid_start defers its channels behind the SDP record
           enumeration, and bt_hid_check_up switches it to REPORT protocol. */
        if (d != NULL) {
            h->is_gamepad = bt_cod_is_gamepad(d->class_of_device);
        }
    }
    return h;
}

/* Accepting a peer's channel must not originate the other half of the pair. */
void bt_hid_accept(l2cap_chan_t* ch) {
    bt_hid_chan_t* h;

    if (ch->psm != L2CAP_PSM_HID_CTRL && ch->psm != L2CAP_PSM_HID_INTR) {
        return;
    }
    {
        bt_device_t* dev = bt_find_device_by_handle(ch->acl_handle);
        h = bt_hid_session_init(ch->acl_handle, dev != NULL ? dev->addr : NULL);
    }
    if (h == NULL) {
        /* both sessions taken: refuse the channel rather than corrupt one */
        l2cap_chan_close(ch, true);
        return;
    }
    if (ch->psm == L2CAP_PSM_HID_CTRL) {
        h->ctrl = ch;
    }
    else {
        h->intr = ch;
    }
}

void bt_hid_start(uint16_t handle, const uint8_t* addr) {
    bt_hid_chan_t* h;

    if (handle == 0) {
        return;
    }
    /* Never bind classic HIDP to an LE handle. An LE peripheral is served by
       HOGP over GATT; classic L2CAP channels cannot ride an LE link, and if
       a session's acl_handle were aliased to a live LE handle the
       duplicate-transport suppression at the end of bt_le_connect would
       disconnect the very HOGP link it is meant to protect (a dual-mode
       gamepad drops right after READY and loops). A dual-mode pad reached
       over LE is refused here; one truly on a classic ACL is not an
       LE-session handle and proceeds normally. */
    if (le_session_by_handle(handle) >= 0) {
        return;
    }
    h = bt_hid_session_init(handle, addr);
    if (h == NULL) {
        return;
    }
    /* A gamepad's channels wait until the SDP record enumeration finished
       (or bt_hid_step's bounded wait gave up): real hosts always enumerate
       first, and the Zikway-firmware pad withholds input from a host that
       opens HID channels without ever having read its record. */
    if (h->is_gamepad && !h->sdp_map_done) {
        if (h->sdp_deadline_ms == 0) {
            h->sdp_deadline_ms = kernel_tic_ms(0) + BT_HID_SDP_MAP_MS;
        }
        if (!h->sdp_map_started) {
            bt_sdp_client_kick(h);
        }
        return;
    }
    /* Enumeration done. A pad whose record says HIDReconnectInitiate opens
       both HID channels itself the moment our SDP channel closes (observed:
       its PSM 0x0011 request lands in the same millisecond as ours). Racing
       it crosses the two control requests, and however the cross is
       resolved this firmware ends up without a working HID binding: the
       zombie channel left by collapsing ours drew invalid-CID rejects, and
       disconnecting it instead killed its HIDP responses and started a
       signaling loop. Real hosts don't race a device that reconnects by
       itself (BlueZ ReconnectMode=device accepts and waits), so give the
       pad a bounded head start; the outbound open below stays as the
       fallback for a pad that waits for the host. */
    if (h->is_gamepad && h->ctrl == NULL) {
        uint64_t now = kernel_tic_ms(0);
        if (h->peer_open_until_ms == 0) {
            h->peer_open_until_ms = now + BT_HID_PEER_OPEN_MS;
        }
        if (now < h->peer_open_until_ms) {
            return;
        }
    }
    if (h->ctrl == NULL || h->ctrl->state != L2CAP_STATE_OPEN) {
        h->ctrl = l2cap_chan_open(handle, L2CAP_PSM_HID_CTRL);
    }
    /* HID 5.2.2: the control-channel initiator also initiates interrupt.
       A peer-initiated reconnect waits for the peer, even on repeated starts
       from an attach poll or security callback. BlueZ follows the same split. */
    if (h->ctrl != NULL && h->ctrl->state == L2CAP_STATE_OPEN &&
            !h->ctrl->incoming &&
            (h->intr == NULL || h->intr->state != L2CAP_STATE_OPEN)) {
        h->intr = l2cap_chan_open(handle, L2CAP_PSM_HID_INTR);
    }
}

/* drop every L2CAP channel (the ACL links die with the radio anyway) */
void bt_hid_stack_reset(void) {
    int i;

    /* the links are being dropped wholesale: let go of anything they hold */
    for (i = 0; i < MAX_CLASSIC_HID_SESSIONS; ++i) {
        if (_hids[i].active) {
            bt_hid_release_held(&_hids[i].held);
        }
    }
    memset(&_l2chans, 0, sizeof(_l2chans));
    memset(&_hids, 0, sizeof(_hids));
    l2cap_rx_reset();
    _acl_credits = 0;
    bt_le_stack_reset();
}
