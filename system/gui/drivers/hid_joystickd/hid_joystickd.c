/*
 * hid_joystickd: Bluetooth/USB gamepad daemon.
 *
 * Structurally a sibling of hid_keybd/hid_moused: it subscribes to one
 * report id on a HID host char device (/dev/hid0 from usbhostd, or /dev/bt0
 * from btd in BT mode), drains the subscriber queue, and re-publishes a
 * normalized event on its own node (/dev/js0, or /dev/js1 for BT).
 *
 * The difference is the payload. The joystick report id (HID_REPORT_ID_
 * JOYSTICK) carries the RAW gamepad report prefix (fixed
 * HID_JOYSTICK_RAW_SIZE stride), and this daemon owns the device-specific
 * decoding -- DualShock 4 (report 0x11), DualSense (report 0x31) and the
 * Xbox GIP input report (0x01) -- exactly as hid_keybd owns the keyboard
 * layout instead of the transport. Each decoded state is normalized into a
 * js_evt_t (hid/hid_joystick.h) and queued for consumers.
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
#include <fcntl.h>
#include <hid/hid_defs.h>
#include <hid/hid_joystick.h>

/* retry cadence while the HID host node is not there yet */
#define HID_CONNECT_SLEEP_US 200000u

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
/* output event queue depth between this daemon and its consumer */
#define JS_CACHE 8
/* cap the drain/normalize pass rate; BT pads report ~125-250Hz */
#define JS_PASS_MS 5u

static int hid = -1;
static const char* _dev_point = "/dev/hid0";
static bool _bt_mode = false;
static fsinfo_t _hid_info;

/* normalized output queue: [0, js_count) hold pending state changes */
static js_evt_t _jsq[JS_CACHE];
static int _js_count = 0;
static uint64_t _last_pass_ms = 0;

/* ------------- normalization ------------- */

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

static uint16_t trig10_to_u16(uint16_t v) {
    uint32_t x = (uint32_t)v << 6; /* 0..1023 -> 0..65472 */
    if (x > 65535u) {
        x = 65535u;
    }
    return (uint16_t)x;
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

/* Xbox GIP standard input report (message type 0x01). Official Xbox pads use
   this framing over their own Bluetooth transport; the layout below is the
   documented little-endian GIP input record. */
static void js_decode_xbox(const uint8_t* r, size_t len, js_evt_t* out) {
    if (len < 20) {
        return;
    }
    uint16_t b0 = (uint16_t)(r[4] | (r[5] << 8));
    uint16_t b1 = (uint16_t)(r[6] | (r[7] << 8));
    uint16_t lt = (uint16_t)(r[8] | (r[9] << 8));
    uint16_t rt = (uint16_t)(r[10] | (r[11] << 8));

    out->lx = (int16_t)(r[12] | (r[13] << 8));
    out->ly = (int16_t)(r[14] | (r[15] << 8));
    out->rx = (int16_t)(r[16] | (r[17] << 8));
    out->ry = (int16_t)(r[18] | (r[19] << 8));
    out->lt = trig10_to_u16(lt);
    out->rt = trig10_to_u16(rt);

    uint32_t b = 0;
    if (b0 & (1u << 4))  b |= JS_BTN_A;
    if (b0 & (1u << 5))  b |= JS_BTN_B;
    if (b0 & (1u << 6))  b |= JS_BTN_X;
    if (b0 & (1u << 7))  b |= JS_BTN_Y;
    if (b0 & (1u << 2))  b |= JS_BTN_START;   /* Menu */
    if (b0 & (1u << 3))  b |= JS_BTN_SELECT;  /* View */
    if (b0 & (1u << 12)) b |= JS_BTN_LB;
    if (b0 & (1u << 13)) b |= JS_BTN_RB;
    if (b0 & (1u << 14)) b |= JS_BTN_LS;
    if (b0 & (1u << 15)) b |= JS_BTN_RS;
    if (b1 & (1u << 0))  b |= JS_BTN_HOME;    /* Guide */
    if (lt > 0x80) b |= JS_BTN_LT;
    if (rt > 0x80) b |= JS_BTN_RT;
    out->buttons = b;

    /* four d-pad bits -> compass hat */
    bool up = b0 & (1u << 8), down = b0 & (1u << 9);
    bool left = b0 & (1u << 10), right = b0 & (1u << 11);
    if (up && right)        out->dpad = JS_DPAD_NE;
    else if (right)         out->dpad = JS_DPAD_E;
    else if (down && right) out->dpad = JS_DPAD_SE;
    else if (down)          out->dpad = JS_DPAD_S;
    else if (down && left)  out->dpad = JS_DPAD_SW;
    else if (left)          out->dpad = JS_DPAD_W;
    else if (up && left)    out->dpad = JS_DPAD_NW;
    else if (up)            out->dpad = JS_DPAD_N;
    else                    out->dpad = JS_DPAD_NONE;
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
        /* Reserved for the Xbox GIP transport: report id 0x05 keeps the GIP
           input report clear of the PlayStation 0x01 it would otherwise
           collide with. Routed once btd can carry GIP frames. */
        js_decode_xbox(raw, len, out);
        break;
    default:
        return false;
    }
    out->connected = 1;
    return true;
}

static bool js_same(const js_evt_t* a, const js_evt_t* b) {
    return a->buttons == b->buttons && a->dpad == b->dpad &&
        a->lx == b->lx && a->ly == b->ly &&
        a->rx == b->rx && a->ry == b->ry &&
        a->lt == b->lt && a->rt == b->rt;
}

/* push a decoded state; drop the oldest when the queue is full. A gamepad
   reports a complete snapshot every frame, so coalescing to the newest is
   lossless for the consumer's view of the current pad state. */
static void js_push(const js_evt_t* evt) {
    if (_js_count >= JS_CACHE) {
        memmove(&_jsq[0], &_jsq[1], (size_t)(_js_count - 1) * sizeof(js_evt_t));
        _js_count--;
    }
    _jsq[_js_count++] = *evt;
}

/* ------------- vdevice callbacks ------------- */

static int js_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* node,
        void* buf, int size, off_t offset, void* p) {
    (void)dev; (void)fd; (void)from_pid; (void)node; (void)offset; (void)p;

    if (size < (int)sizeof(js_evt_t)) {
        return -1;
    }
    if (_js_count <= 0) {
        return VFS_ERR_RETRY;
    }
    memcpy(buf, &_jsq[0], sizeof(js_evt_t));
    memmove(&_jsq[0], &_jsq[1], (size_t)(_js_count - 1) * sizeof(js_evt_t));
    _js_count--;
    return (int)sizeof(js_evt_t);
}

static uint32_t js_check_poll_events(vdevice_t* dev, int fd, int from_pid,
        fsinfo_t* node, void* p) {
    (void)dev; (void)fd; (void)from_pid; (void)node; (void)p;
    return (_js_count > 0) ? VFS_EVT_RD : 0;
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
    if (_bt_mode) {
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

    /* drain every queued raw report in one pass; each event is a fixed
       HID_JOYSTICK_RAW_SIZE stride (see bt_hid_dispatch_joystick) */
    ipc_disable();
    bool failed = false;
    uint8_t buf[JS_DRAIN_SIZE];
    while (true) {
        int res = read(hid, buf, sizeof(buf));
        if (res >= HID_JOYSTICK_RAW_SIZE) {
            for (int off = 0; off + HID_JOYSTICK_RAW_SIZE <= res;
                    off += HID_JOYSTICK_RAW_SIZE) {
                js_evt_t evt;
                if (js_normalize(buf + off, HID_JOYSTICK_RAW_SIZE, &evt)) {
                    /* only queue a state change; a pad streams identical
                       snapshots while idle and coalescing them keeps the
                       consumer's queue meaningful */
                    if (_js_count == 0 || !js_same(&_jsq[_js_count - 1], &evt)) {
                        js_push(&evt);
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

    if (failed) {
        close(hid);
        hid = -1;
        memset(&_hid_info, 0, sizeof(fsinfo_t));
        _js_count = 0;
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
                bt_try_attach_hid();
            }
        }
    }

    /* level-triggered wakeup: keep feeding a blocked consumer while the
       output queue still holds unread state */
    if (_js_count > 0) {
        vfs_wakeup(dev->mnt_info.node, VFS_EVT_RD);
    }
    return 0;
}

int main(int argc, char** argv) {
    const char* mnt_point = argc > 1 ? argv[1] : "/dev/js0";
    if (argc > 2) {
        _dev_point = argv[2];
    }
    /* argv[3] == "bt": the upstream is btd (e.g. /dev/bt0), run in BT mode */
    if (argc > 3 && strcmp(argv[3], "bt") == 0) {
        _bt_mode = true;
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
