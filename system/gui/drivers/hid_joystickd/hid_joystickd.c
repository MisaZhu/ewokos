/*
 * hid_joystickd: Bluetooth/USB gamepad daemon.
 *
 * Structurally a sibling of hid_keybd/hid_moused: it subscribes to one
 * report id on a HID host char device (/dev/hid0 from usbhostd, or /dev/bt0
 * from btd in BT mode), drains the subscriber queue, and re-publishes the
 * gamepad state on its own node (/dev/joystick0, or /dev/js1 for BT).
 *
 * Upstream payload. The joystick report id (HID_REPORT_ID_JOYSTICK) carries
 * a fixed HID_JOYSTICK_RAW_SIZE-stride frame whose content depends on the
 * transport:
 *   - BT mode (argv[3] == "bt", upstream btd): either the RAW gamepad report
 *     prefix, decoded here by leading report id -- DualShock 4 (0x11),
 *     DualSense (0x31) and the PS truncated default (0x01) -- or, for a BLE
 *     HOGP pad (Xbox/8BitDo) whose Report Map only btd ever sees, an
 *     already-normalized js_evt_t carried behind the reserved 0x05 tag.
 *   - USB mode (upstream usbhostd): an already-normalized js_evt_t, because
 *     usbhostd is the only place that ever sees the USB report descriptor
 *     and runs the descriptor-driven generic gamepad parser. This daemon
 *     just passes those frames through.
 * Either way each frame is normalized into a js_evt_t (hid/hid_joystick.h).
 *
 * Downstream contract. This node is read EXACTLY like every other key-ish
 * EwokOS input device (gpio_joystickd, hid_keybd): read(fd, buf, size)
 * returns a snapshot of the currently-"pressed" key codes -- one raw
 * uint8_t per key, up to `size` bytes -- or VFS_ERR_RETRY when nothing is
 * active. The js_evt_t is translated into KEY_UP/DOWN/LEFT/RIGHT (from the
 * d-pad hat, or from the left analog stick for pads like the uConsole that
 * drive their d-pad through Joystick.X/Y) plus the JOYSTICK_* button codes.
 * That is what vkeybd -t j, vjoystickd, xmouse and xim_none all consume, so
 * the gamepad drops straight into the proven handheld input chain instead
 * of a private js_evt_t format nothing reads.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ewoksys/vfs.h>
#include <ewoksys/klog.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/proc.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/keydef.h>
#include <fcntl.h>
#include <hid/hid_defs.h>
#include <hid/hid_joystick.h>

/* retry cadence while the HID host node is not there yet */
#define HID_CONNECT_SLEEP_US 200000u

/* re-check cadence while keys are held: a gamepad that honours Set_Idle goes
   silent between the press and the release report, so an unbounded kernel
   block would stall the level-triggered wakeups of /dev/joystick0 readers.
   5ms keeps the held-snapshot re-publish tight enough that a missed directed
   wake recovers in one tick instead of four. */
#define HID_WAIT_FALLBACK_US 5000u

/*
 * BT mode (argv[3] == "bt"): the upstream is btd's /dev/bt0. The subscriber
 * protocol is identical to hid_keybd's BT mode; the deltas are the bounded
 * idle wait (an idle pad produces zero traffic, so the daemon must wake on
 * its own to run the attach poller) and the periodic hid_open attempt.
 */
#define BT_WAIT_REPORT_US 500000u
#define BT_ATTACH_POLL_MS 2000u

/* drain one read sized for the whole subscriber queue in a single round-trip */
#define JS_DRAIN_SIZE (HID_QUEUE_DEPTH * HID_JOYSTICK_RAW_SIZE)

/* cap the drain/normalize pass rate; aligned with btd's 2ms poll floor so
   the consumer never adds latency on top of the transport */
#define JS_PASS_MS 2u

/*
 * Key-code snapshot capacity. Consumers (vkeybd -t j, vjoystickd) read only
 * KEY_NUM==4 bytes at a time, so the first four active keys are what reaches
 * the UI; the extra room keeps the transient-tap latch lossless.
 */
#define MAX_JS_KEY 12

/* half-scale deadzone for treating an analog stick as a d-pad direction */
#define JS_AXIS_THRESH 16384
/* analog trigger pull (0..65535) that registers as a digital L2/R2 */
#define JS_TRIG_THRESH 0x8000u

static int hid = -1;
static const char* _dev_point = "/dev/hid0";
static bool _bt_mode = false;
static fsinfo_t _hid_info;

/* current held-key snapshot, refreshed from the newest decoded js_evt_t */
static uint8_t _keys[MAX_JS_KEY];
static int _key_count = 0;

/*
 * Transient ("tap") keys, mirroring hid_keybd. One drain pass spans several
 * frames and a button pressed AND released inside that burst is already gone
 * from the newest snapshot; exposing only the newest would drop the tap
 * entirely (the reader never sees the press). Latch such keys and expose them
 * on exactly ONE read so the consumer diffs a clean press then a release.
 */
static uint8_t _tap_keys[MAX_JS_KEY];
static int _tap_count = 0;

/*
 * Full-release latch. The downstream consumer (xim_none via keyb.c) diffs
 * CONSECUTIVE reads and only emits a key RELEASE once it observes a snapshot
 * where that key is absent. When the last held key goes up, _key_count drops
 * to 0; if js_read then returns VFS_ERR_RETRY the blocking reader parks and
 * never observes the empty snapshot, so keyb.c leaves the key stuck in its
 * HOLD state -- the next press of the SAME key produces no fresh edge (only a
 * DIFFERENT key, forcing a diff, unsticks it). Set this on the active->idle
 * transition and serve exactly ONE empty (0-length) read so keyb.c emits the
 * deferred release, then park again.
 */
static bool _release_pending = false;

/* timestamp of the last drain pass, for the JS_PASS_MS rate cap */
static uint64_t _last_pass_ms = 0;

/* ------------- normalization (raw report prefix -> js_evt_t) ------------- */

static int16_t axis8_to_i16(uint8_t v) {
    int x = ((int)v - 128) << 8; /* -32768..32512 */
    if (x > 32767) {
        x = 32767;
    }
    return (int16_t)x;
}

static uint16_t trig8_to_u16(uint8_t v) {
    return (uint16_t)((unsigned)v * 257u); /* 0..255 -> 0..65535 */
}

/*
 * PlayStation face/shoulder/system button encoding, shared by every PS input
 * report. `lo` carries the d-pad hat (low nibble) and the four face buttons
 * (high nibble), `hi` the bumpers/triggers/select/start/stick-clicks and
 * `sys` the PS/touchpad bits. The mic-mute bit only exists on the DualSense
 * expanded report, so callers add JS_BTN_MUTE themselves.
 */
static uint32_t ps_buttons(uint8_t lo, uint8_t hi, uint8_t sys) {
    uint32_t b = 0;
    if (lo & 0x10) b |= JS_BTN_X;      /* Square  */
    if (lo & 0x20) b |= JS_BTN_A;      /* Cross   */
    if (lo & 0x40) b |= JS_BTN_B;      /* Circle  */
    if (lo & 0x80) b |= JS_BTN_Y;      /* Triangle*/
    if (hi & 0x01) b |= JS_BTN_LB;     /* L1 */
    if (hi & 0x02) b |= JS_BTN_RB;     /* R1 */
    if (hi & 0x04) b |= JS_BTN_LT;     /* L2 digital */
    if (hi & 0x08) b |= JS_BTN_RT;     /* R2 digital */
    if (hi & 0x10) b |= JS_BTN_SELECT; /* Share / Create */
    if (hi & 0x20) b |= JS_BTN_START;  /* Options */
    if (hi & 0x40) b |= JS_BTN_LS;     /* L3 */
    if (hi & 0x80) b |= JS_BTN_RS;     /* R3 */
    if (sys & 0x01) b |= JS_BTN_HOME;  /* PS */
    if (sys & 0x02) b |= JS_BTN_TOUCH; /* touchpad click */
    return b;
}

/*
 * PlayStation truncated/default input report 0x01 (10 bytes). This is the
 * layout a DualShock 4 AND a DualSense both stream by default over Bluetooth;
 * the expanded reports (0x11 / 0x31) only appear after an explicit feature
 * report enables them, which this stack does not issue -- so 0x01 is the real
 * runtime path for every PS pad.
 *   [1..4] LX LY RX RY, [5] dpad+face, [6] bumpers.., [7] PS/touch, [8..9] L2 R2
 */
static void js_decode_ps_truncated(const uint8_t* r, size_t len, js_evt_t* out) {
    if (len < 10) {
        return;
    }
    out->lx = axis8_to_i16(r[1]);
    out->ly = axis8_to_i16(r[2]);
    out->rx = axis8_to_i16(r[3]);
    out->ry = axis8_to_i16(r[4]);
    out->lt = trig8_to_u16(r[8]);
    out->rt = trig8_to_u16(r[9]);
    out->dpad = (uint8_t)(r[5] & 0x0f);
    out->buttons = ps_buttons(r[5], r[6], r[7]);
}

/*
 * PlayStation EXPANDED reports: DualShock 4 (0x11) and DualSense (0x31). Same
 * button encoding as the truncated report but a different field order -- the
 * DualSense packs the analog triggers before the buttons, the DualShock 4
 * puts them after. `off`-style offset tables handle both. Only seen when an
 * expanded report was explicitly enabled; harmless to support for robustness.
 */
static void js_decode_ps(const uint8_t* r, size_t len, js_evt_t* out, bool ds5) {
    uint8_t lx, ly, rx, ry, lo, hi, sys, lt, rt;

    if (ds5) {
        if (len < 11) {
            return;
        }
        lx = r[2]; ly = r[3]; rx = r[4]; ry = r[5];
        lt = r[6]; rt = r[7];
        lo = r[8]; hi = r[9]; sys = r[10];
    }
    else {
        if (len < 12) {
            return;
        }
        lx = r[3]; ly = r[4]; rx = r[5]; ry = r[6];
        lo = r[7]; hi = r[8]; sys = r[9];
        lt = r[10]; rt = r[11];
    }

    out->lx = axis8_to_i16(lx);
    out->ly = axis8_to_i16(ly);
    out->rx = axis8_to_i16(rx);
    out->ry = axis8_to_i16(ry);
    out->lt = trig8_to_u16(lt);
    out->rt = trig8_to_u16(rt);

    /* d-pad hat: 0..7 compass, 8 released -- identical to JS_DPAD_* */
    out->dpad = (uint8_t)(lo & 0x0f);

    uint32_t b = ps_buttons(lo, hi, sys);
    if (ds5 && (sys & 0x04)) b |= JS_BTN_MUTE;
    out->buttons = b;
}

/* decode one raw report prefix; returns false when the report id is unknown */
static bool js_normalize(const uint8_t* raw, size_t len, js_evt_t* out) {
    memset(out, 0, sizeof(*out));
    out->dpad = JS_DPAD_NONE;
    if (len < 1) {
        return false;
    }
    switch (raw[0]) {
    case 0x01:
        /* the report every PlayStation pad streams by default (DualShock 4
           and DualSense share the truncated 10-byte layout) */
        js_decode_ps_truncated(raw, len, out);
        break;
    case 0x11:
        js_decode_ps(raw, len, out, false); /* DualShock 4 expanded */
        break;
    case 0x31:
        js_decode_ps(raw, len, out, true);  /* DualSense expanded */
        break;
    case 0x05:
        /* btd's LE HOGP path is the only place that ever sees the Report Map
           of a BLE gamepad (an Xbox/8BitDo pad), so it runs libhid's
           descriptor-driven normalizer itself and forwards the resulting
           js_evt_t prefixed by this reserved tag. Pass it straight through:
           the classic PS reports above still arrive raw and decode by their
           own leading report id. */
        if (len < 1 + sizeof(js_evt_t)) {
            return false;
        }
        memcpy(out, raw + 1, sizeof(js_evt_t));
        break;
    default:
        return false;
    }
    out->connected = 1;
    return true;
}

/*
 * USB mode: usbhostd has already run the descriptor-driven joystick parser
 * (it is the only place that ever sees the report descriptor), so the frame
 * it dispatches on HID_REPORT_ID_JOYSTICK is an already-normalized js_evt_t
 * zero-padded to the fixed stride. Pass it straight through.
 */
static bool js_decode_normalized(const uint8_t* raw, js_evt_t* out) {
    memset(out, 0, sizeof(*out));
    memcpy(out, raw, sizeof(js_evt_t));
    return out->connected != 0;
}

/* ------------- js_evt_t -> pressed key-code snapshot ------------- */

/*
 * Translate a normalized gamepad state into the list of key codes a UI reader
 * expects (the same alphabet gpio_joystickd emits). Directions come from the
 * d-pad hat when the pad has one; pads that drive their d-pad through the
 * left analog stick (the uConsole keyboard's gamepad cluster sends
 * Joystick.X/Y) fall back to an axis deadzone. Buttons map onto the
 * JOYSTICK_* codes vkeybd/vjoystickd already understand. Returns the number
 * of key codes written (0 when the pad is idle), capped at `max`.
 */
static int js_to_keys(const js_evt_t* e, uint8_t* keys, int max) {
    int n = 0;
    int up = 0, down = 0, left = 0, right = 0;

    switch (e->dpad) {
    case JS_DPAD_N:  up = 1; break;
    case JS_DPAD_S:  down = 1; break;
    case JS_DPAD_E:  right = 1; break;
    case JS_DPAD_W:  left = 1; break;
    case JS_DPAD_NE: up = 1; right = 1; break;
    case JS_DPAD_NW: up = 1; left = 1; break;
    case JS_DPAD_SE: down = 1; right = 1; break;
    case JS_DPAD_SW: down = 1; left = 1; break;
    default: break; /* JS_DPAD_NONE */
    }
    if (!up && !down && !left && !right) {
        if (e->ly < -JS_AXIS_THRESH)      up = 1;
        else if (e->ly > JS_AXIS_THRESH)  down = 1;
        if (e->lx < -JS_AXIS_THRESH)      left = 1;
        else if (e->lx > JS_AXIS_THRESH)  right = 1;
    }

    if (up    && n < max) keys[n++] = KEY_UP;
    if (down  && n < max) keys[n++] = KEY_DOWN;
    if (left  && n < max) keys[n++] = KEY_LEFT;
    if (right && n < max) keys[n++] = KEY_RIGHT;

    uint32_t b = e->buttons;
    if ((b & JS_BTN_A) && n < max) keys[n++] = JOYSTICK_A;
    if ((b & JS_BTN_B) && n < max) keys[n++] = JOYSTICK_B;
    if ((b & JS_BTN_X) && n < max) keys[n++] = JOYSTICK_X;
    if ((b & JS_BTN_Y) && n < max) keys[n++] = JOYSTICK_Y;
    if ((b & JS_BTN_LB) && n < max) keys[n++] = JOYSTICK_L1;
    if ((b & JS_BTN_RB) && n < max) keys[n++] = JOYSTICK_R1;
    if (((b & JS_BTN_LT) || e->lt > JS_TRIG_THRESH) && n < max) keys[n++] = JOYSTICK_L2;
    if (((b & JS_BTN_RT) || e->rt > JS_TRIG_THRESH) && n < max) keys[n++] = JOYSTICK_R2;
    if ((b & JS_BTN_SELECT) && n < max) keys[n++] = JOYSTICK_SELECT;
    if ((b & JS_BTN_START) && n < max) keys[n++] = JOYSTICK_START;
    if ((b & JS_BTN_LS) && n < max) keys[n++] = JOYSTICK_THUMBL;
    if ((b & JS_BTN_RS) && n < max) keys[n++] = JOYSTICK_THUMBR;
    if ((b & JS_BTN_HOME) && n < max) keys[n++] = KEY_HOME;
    return n;
}

/* ------------- vdevice callbacks ------------- */

static int js_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* node,
        void* buf, int size, off_t offset, void* p) {
    (void)dev; (void)fd; (void)from_pid; (void)node; (void)offset; (void)p;

    if (size <= 0) {
        return -1;
    }
    /* keep returning the held snapshot until the loop observes a change, and
       expose each latched transient tap exactly once (mirrors hid_keybd) */
    int num = 0;
    uint8_t* out = (uint8_t*)buf;
    for (int i = 0; i < _key_count && num < size; i++) {
        out[num++] = _keys[i];
    }
    if (_tap_count > 0) {
        for (int i = 0; i < _tap_count && num < size; i++) {
            out[num++] = _tap_keys[i];
        }
        _tap_count = 0;
    }
    if (num > 0) {
        return num;
    }
    /* idle: hand the blocking consumer ONE empty snapshot after a full
       release so keyb.c can emit the deferred RELEASE, then park again */
    if (_release_pending) {
        _release_pending = false;
        return 0;
    }
    return VFS_ERR_RETRY;
}

static uint32_t js_check_poll_events(vdevice_t* dev, int fd, int from_pid,
        fsinfo_t* node, void* p) {
    (void)dev; (void)fd; (void)from_pid; (void)node; (void)p;
    return (_key_count > 0 || _tap_count > 0 || _release_pending) ?
            VFS_EVT_RD : 0;
}

/* ------------- transport (mirrors hid_keybd) ------------- */

static int set_report_id(int fd, int id) {
    proto_t in;
    PF->init(&in)->addi(&in, id);
    int ret = vfs_fcntl(fd, 0, &in, NULL);
    PF->clear(&in);
    return ret;
}

static bool hid_connect(void) {
    int fd;

    if (hid >= 0) {
        return true;
    }
    fd = open(_dev_point, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        return false;
    }
    if (set_report_id(fd, HID_REPORT_ID_JOYSTICK) != 0) {
        close(fd);
        return false;
    }
    if (vfs_get_by_fd(fd, &_hid_info) != 0 || _hid_info.node == 0) {
        close(fd);
        return false;
    }
    /* register on the node's read wait queue once and keep it permanently */
    proto_t in;
    PF->init(&in)->
        addi(&in, _hid_info.node)->
        addi(&in, VFS_EVT_RD);
    ipc_call(get_vfsd_pid(), VFS_BLOCK, &in, NULL);
    PF->clear(&in);
    hid = fd;
    return true;
}

static void hid_wait_report(void) {
    if (_key_count > 0) {
        /* keys held: bounded cadence so level-triggered readers keep seeing
           the held snapshot even when the pad sends no repeat reports */
        proc_block_timeout(_hid_info.node, HID_WAIT_FALLBACK_US);
    }
    else if (_bt_mode) {
        /* bounded idle wait: an idle pad produces no wakeup traffic, so the
           daemon must wake on its own to run the attach poller */
        proc_block_timeout(_hid_info.node, BT_WAIT_REPORT_US);
    }
    else {
        proc_block_by(_hid_info.node);
    }
}

/* BT mode only: open a HID session on the first connected gamepad. Mirrors
   hid_keybd's attach poller but filters the CoD for the joystick/gamepad
   minor bits instead of the keyboard/pointing ones. */
static void bt_try_attach_hid(void) {
    char* list = dev_cmd(_dev_point, "devices");
    if (list == NULL) {
        return;
    }

    char* save = NULL;
    for (char* line = strtok_r(list, "\n", &save); line != NULL;
            line = strtok_r(NULL, "\n", &save)) {
        char addr[24];
        unsigned int cod = 0;
        int connected = 0;
        int le = 0;

        if (strstr(line, "device ") == NULL) {
            continue;
        }
        if (sscanf(line, "%*d: device %23s class=0x%x rssi=%*d connected=%d paired=%*d le=%d",
                addr, &cod, &connected, &le) != 4) {
            continue;
        }
        if (!connected) {
            continue;
        }
        /* Peripheral major (0x05) with the joystick/gamepad minor bits, or any
           LE device (hid_open on an LE address drives the HOGP bring-up) */
        if (!((((cod >> 8) & 0x1f) == 0x05 && (cod & 0x0c) != 0) || le)) {
            continue;
        }

        char cmd[40];
        snprintf(cmd, sizeof(cmd), "hid_open %s", addr);
        char* ret = dev_cmd(_dev_point, cmd);
        printf("hid_joystickd: attach %s: %s\n", addr, ret != NULL ? ret : "no_reply");
        if (ret != NULL) {
            free(ret);
        }
        break;
    }
    free(list);
}

static int loop(vdevice_t* dev, void* p) {
    (void)p;

    if (!hid_connect()) {
        usleep(HID_CONNECT_SLEEP_US);
        return 0;
    }

    hid_wait_report();

    /* rate cap so a fast pad cannot drive the loop above 1/JS_PASS_MS */
    uint64_t now = kernel_tic_ms(0);
    uint64_t next = _last_pass_ms + JS_PASS_MS;
    if (now < next) {
        usleep((uint32_t)((next - now) * 1000u));
    }

    /*
     * Drain every queued frame in one pass; each event is a fixed
     * HID_JOYSTICK_RAW_SIZE stride. BT mode carries the raw gamepad report
     * prefix (decoded here by report id); USB mode carries an already
     * normalized js_evt_t from usbhostd (passed straight through). The NEWEST
     * frame is the current held state; the union of every frame's key codes
     * feeds the transient-tap latch so a quick press/release is not collapsed
     * away.
     */
    ipc_disable();
    bool failed = false;
    uint8_t burst_keys[MAX_JS_KEY];
    int burst_count = 0;
    bool got_state = false;
    int prev_key_count = _key_count;
    uint8_t buf[JS_DRAIN_SIZE];
    while (true) {
        int res = read(hid, buf, sizeof(buf));
        if (res >= HID_JOYSTICK_RAW_SIZE) {
            for (int off = 0; off + HID_JOYSTICK_RAW_SIZE <= res;
                    off += HID_JOYSTICK_RAW_SIZE) {
                js_evt_t evt;
                bool ok = _bt_mode ?
                        js_normalize(buf + off, HID_JOYSTICK_RAW_SIZE, &evt) :
                        js_decode_normalized(buf + off, &evt);
                if (!ok) {
                    continue;
                }
                uint8_t k[MAX_JS_KEY];
                int kc = js_to_keys(&evt, k, MAX_JS_KEY);
                _key_count = kc;
                memcpy(_keys, k, (size_t)kc);
                got_state = true;
                for (int i = 0; i < kc && burst_count < MAX_JS_KEY; i++) {
                    bool dup = false;
                    for (int j = 0; j < burst_count; j++) {
                        if (burst_keys[j] == k[i]) {
                            dup = true;
                            break;
                        }
                    }
                    if (!dup) {
                        burst_keys[burst_count++] = k[i];
                    }
                }
            }
            if (res < (int)sizeof(buf)) {
                break; /* short batch: the queue ran dry */
            }
            continue;
        }
        if (res < 0 && errno != EAGAIN) {
            failed = true; /* node gone / usbhostd restarted: reconnect */
        }
        break;
    }
    ipc_enable();
    _last_pass_ms = kernel_tic_ms(0);

    /* latch taps: keys seen during the burst but absent from the newest
       snapshot; only replace a still-pending latch when this burst made taps
       so a later empty drain cannot clear an unread one */
    if (!failed && got_state && burst_count > 0) {
        uint8_t taps[MAX_JS_KEY];
        int n = 0;
        for (int i = 0; i < burst_count && n < MAX_JS_KEY; i++) {
            bool held = false;
            for (int j = 0; j < _key_count; j++) {
                if (_keys[j] == burst_keys[i]) {
                    held = true;
                    break;
                }
            }
            if (!held) {
                taps[n++] = burst_keys[i];
            }
        }
        if (n > 0) {
            memcpy(_tap_keys, taps, (size_t)n);
            _tap_count = n;
        }
    }

    if (failed) {
        close(hid);
        hid = -1;
        memset(&_hid_info, 0, sizeof(fsinfo_t));
        /* the upstream is gone (btd/usbhostd restarted): whatever it last
           showed as held is released, and the consumer must see that too */
        if (_key_count > 0) {
            _release_pending = true;
        }
        _key_count = 0;
        _tap_count = 0;
        if (_release_pending) {
            vfs_wakeup(dev->mnt_info.node, VFS_EVT_RD);
        }
        usleep(HID_CONNECT_SLEEP_US);
        return 0;
    }

    /* BT mode: keep trying to attach a session while none is up */
    if (_bt_mode) {
        static uint64_t last_attach_ms = 0;
        uint64_t tnow = kernel_tic_ms(0);
        if (tnow - last_attach_ms >= BT_ATTACH_POLL_MS) {
            last_attach_ms = tnow;
            char* st = dev_cmd(_dev_point, "hid_state");
            bool session_up = st != NULL && strstr(st, "active=1") != NULL;
            if (st != NULL) {
                free(st);
            }
            if (!session_up) {
                /*
                 * No link, so nothing can be held: a snapshot left over from
                 * a pad that dropped mid-press would otherwise keep
                 * /dev/joystick0 readable forever and the consumer (keyb.c
                 * under xim_none) auto-repeating a button nobody is pressing.
                 * btd synthesizes an all-released report for keyboard/mouse
                 * on teardown but NOT for the joystick, so this is the
                 * backstop that clears the stale held state and arms one
                 * empty read so the deferred RELEASE reaches X. A latched tap
                 * is left alone: it is a real press and clears on one read.
                 */
                if (_key_count > 0) {
                    _key_count = 0;
                    _release_pending = true;
                }
                bt_try_attach_hid();
            }
        }
    }

    /* full release (had keys, now none): latch so the next idle read serves
       one empty snapshot and the diffing consumer emits the deferred RELEASE */
    if (!failed && got_state && prev_key_count > 0 && _key_count == 0) {
        _release_pending = true;
    }

    /* level-triggered wakeup: keep feeding a blocked consumer while keys are
       held, a transient tap is still pending, or a release must be delivered */
    if (_key_count > 0 || _tap_count > 0 || _release_pending) {
        vfs_wakeup(dev->mnt_info.node, VFS_EVT_RD);
    }
    return 0;
}

int main(int argc, char** argv) {
    const char* mnt_point = argc > 1 ? argv[1] : "/dev/joystick0";
    if (argc > 2) {
        _dev_point = argv[2];
    }
    /* argv[3] == "bt": the upstream is btd (e.g. /dev/bt0), run in BT mode */
    if (argc > 3 && strcmp(argv[3], "bt") == 0) {
        _bt_mode = true;
    }
    if (_bt_mode) {
        slog("js input_start node=%s src=%s report_id=%u\n",
            mnt_point, _dev_point, (unsigned)HID_REPORT_ID_JOYSTICK);
    }

    vdevice_t dev;
    memset(&dev, 0, sizeof(vdevice_t));
    strcpy(dev.desc, "joystick");
    dev.read = js_read;
    dev.loop_step = loop;
    dev.check_poll_events = js_check_poll_events;

    device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444, false);
    return 0;
}
