/*
 * xtouch: touch input client for the X server.
 *
 * Two modes, selected by -m:
 *
 *  default ....... single-touch ABSOLUTE mode: reads packed [state,x,y]
 *                  samples from /dev/touch0 (hid_touchd) and maps them onto
 *                  the whole screen (touchscreen semantics: touch == left
 *                  click at the touched point).
 *
 *  -m ............. Precision Touchpad (PTP) mode: reads multi-contact
 *                  touchpad_evt_t frames (58 bytes) from /dev/touchpad0
 *                  (hid_touchpadd) and converts them into mouse events.
 *                  Only the FIRST down contact is tracked; any other
 *                  fingers are ignored:
 *
 *                    contact stays still (< tap_move logical units):
 *                        lifted < 300 ms .......... single click
 *                        held   >= 300 ms ......... drag: synthetic LEFT
 *                                                   DOWN, motion drags,
 *                                                   lift releases
 *                    contact moves past tap_move ... plain pointer move
 *                                                   (no button)
 *                    physical buttons ............. LEFT/RIGHT/MID clicks;
 *                                                   holding one while moving
 *                                                   drags
 *
 *  The still/hold decision is driven by a timer in the main loop, so it
 *  works even when the pad stops reporting while a finger is parked.
 *  Double taps are folded into a single click.
 *
 *  Contacts are tracked by the pad's logical ranges (x_max/y_max carried
 *  in the frame) for pointer scaling. Events go to xserverd through
 *  dev_cntl_by_pid(X_DCNTL_INPUT), the same channel xtouch/xmouse/xim use.
 *
 *  Config is optional JSON: /etc/x/xtouch.json for absolute mode,
 *  /etc/x/xtouchpad.json for PTP mode (see xtouchpad.json under
 *  system/xwin/etc/x). -c overrides the path in either mode.
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/core.h>
#include <ewoksys/keydef.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/vfs.h>
#include <tinyjson/tinyjson.h>
#include <ewoksys/proto.h>
#include <x/xcntl.h>
#include <x/xevent.h>
#include <x/xwin.h>
#include <mouse/mouse.h>
#include <hid/hid_touchpad.h>

static int _x_pid = -1;
static int _scr_w = 800;
static int _scr_h = 480;

/* ================= single-touch absolute mode ================= */

typedef struct {
    bool x_rev;
    bool y_rev;
    bool xy_switch;
    int  x_offset;
    int  y_offset;
    int  x_max;
    int  y_max;
} xtouch_t;

static xtouch_t _xtouch;
static uint64_t _drag_time = 0;
static const char* _default_config = "/etc/x/xtouch.json";
static const char* _default_touch0 = "/dev/touch0";

static void input(uint16_t state, int16_t tx, int16_t ty) {
    xevent_t ev;
    memset(&ev, 0, sizeof(xevent_t));
    ev.type = XEVT_MOUSE;
    ev.state = MOUSE_STATE_MOVE;
    ev.value.mouse.relative = 0;
    if(_xtouch.x_rev)
        ev.value.mouse.x = _scr_w - (tx*_scr_w / _xtouch.x_max);
    else
        ev.value.mouse.x = (tx*_scr_w / _xtouch.x_max);

    if(_xtouch.y_rev)
        ev.value.mouse.y = (ty*_scr_h / _xtouch.y_max);
    else
        ev.value.mouse.y = _scr_h - (ty*_scr_h / _xtouch.y_max);

    if(state == 1) { //down
        ev.value.mouse.button = MOUSE_BUTTON_LEFT;
        if(_drag_time == 0) {
            ev.state = MOUSE_STATE_DOWN;
            _drag_time = kernel_tic_ms(0);
        }
        else {
            static int prev_x = 0;
            static int prev_y = 0;
            if((kernel_tic_ms(0) - _drag_time) < 100 ||
                    (ev.value.mouse.x == prev_x || ev.value.mouse.y == prev_y))
                return;
            prev_x = ev.value.mouse.x;
            prev_y = ev.value.mouse.y;
            ev.state = MOUSE_STATE_DRAG;
        }
    } else if(state == 0) { //up
        ev.value.mouse.button = MOUSE_BUTTON_LEFT;
        ev.state = MOUSE_STATE_UP;
        _drag_time = 0;
    }

    proto_t in;
    PF->init(&in)->add(&in, &ev, sizeof(xevent_t));
    dev_cntl_by_pid(_x_pid, X_DCNTL_INPUT, &in, NULL);
    PF->clear(&in);
}

static int32_t read_config(const char* fname) {
    memset(&_xtouch, 0, sizeof(xtouch_t));
    _xtouch.x_max = 100;
    _xtouch.y_max = 100;

    json_var_t *conf_var = json_parse_file(fname);

    _xtouch.x_rev = json_get_bool_def(conf_var, "x_rev", false);
    _xtouch.y_rev = json_get_bool_def(conf_var, "y_rev", false);
    _xtouch.xy_switch = json_get_bool_def(conf_var, "xy_switch", false);

    _xtouch.x_max = json_get_int_def(conf_var, "x_max", 0);
    _xtouch.y_max = json_get_int_def(conf_var, "y_max", 0);
    _xtouch.x_offset = json_get_int_def(conf_var, "x_offset", 0);
    _xtouch.y_offset = json_get_int_def(conf_var, "y_offset", 0);

    if(conf_var != NULL)
        json_var_unref(conf_var);
    return 0;
}

/* ================= PTP mode (-m) ================= */

#define TP_CONTACT_ID_MAX 16
#define TP_DEFAULT_CONFIG "/etc/x/xtouchpad.json"
#define TP_DEFAULT_DEV "/dev/touchpad0"

typedef struct {
    bool tap_enable;
    int  tap_time;      /* ms: still-hold that long starts a drag; a lift
                           sooner is a click */
    int  tap_move;      /* logical units: still vs moving threshold */
    int  move_threshold;/* logical units: travel past this switches WAIT
                           into pointer-move mode (default 20) */
    float gain_x, gain_y; /* pointer speed multiplier */
} tp_cfg_t;

static tp_cfg_t _cfg;
static bool _mouse_mode = false;

static void tp_load_config(const char* fname) {
    memset(&_cfg, 0, sizeof(_cfg));
    _cfg.tap_enable = true;
    _cfg.tap_time = 300;
    _cfg.tap_move = 5;      /* ~5 logical units counts as "still" */
    _cfg.move_threshold = 20; /* travel past this counts as "moving" */
    _cfg.gain_x = 1.0f;
    _cfg.gain_y = 1.0f;

    json_var_t* conf = json_parse_file(fname);
    if (conf == NULL)
        return;

    _cfg.tap_enable = json_get_bool_def(conf, "tap_enable", _cfg.tap_enable);
    _cfg.tap_time = json_get_int_def(conf, "tap_time", _cfg.tap_time);
    _cfg.tap_move = json_get_int_def(conf, "tap_move", _cfg.tap_move);
    _cfg.move_threshold = json_get_int_def(conf, "move_threshold", _cfg.move_threshold);
    _cfg.gain_x = json_get_float_def(conf, "gain_x", _cfg.gain_x);
    _cfg.gain_y = json_get_float_def(conf, "gain_y", _cfg.gain_y);

    json_var_unref(conf);
}

/* the "still" radius in logical units; <= 0 in the config falls back to a
   small fraction of the pad's logical width */
static int tp_tap_move(int x_max) {
    int v = _cfg.tap_move;
    if (v <= 0) {
        v = x_max > 0 ? x_max / 64 : 16;
        if (v < 4)
            v = 4;
    }
    return v;
}

/* travel past this switches a still-pending contact into pointer-move
   mode (debounce: small wiggles while deciding never move the pointer) */
static int tp_move_threshold(int x_max) {
    int v = _cfg.move_threshold;
    if (v <= 0) {
        v = x_max > 0 ? x_max / 64 : 16;
        if (v < 4)
            v = 4;
    }
    return v;
}

/* ---------------- X server plumbing ---------------- */

static void tp_send(xevent_t* ev) {
    proto_t in;
    PF->init(&in)->add(&in, ev, sizeof(xevent_t));
    if (dev_cntl_by_pid(_x_pid, X_DCNTL_INPUT, &in, NULL) != 0) {
        _x_pid = -1;
    }
    PF->clear(&in);
}

/* Re-acquire the X server after a restart: tp_send() clears _x_pid on a
   failed delivery, and the server pid (plus the screen size, which may
   change with it) must not stay stale forever. Throttled to a light
   poll so a dead server costs nothing per frame. */
static void ensure_x_server(void) {
    static uint64_t last_ms = 0;
    uint64_t now;
    if (_x_pid > 0)
        return;
    now = kernel_tic_ms(0);
    if (now - last_ms < 500)
        return;
    last_ms = now;
    _x_pid = dev_get_pid("/dev/x");
    if (_x_pid > 0) {
        xscreen_info_t scr;
        x_screen_info(&scr, 0);
        if (scr.size.w > 0)
            _scr_w = scr.size.w;
        if (scr.size.h > 0)
            _scr_h = scr.size.h;
    }
}

/* one pointer event: state MOUSE_STATE_*, button MOUSE_BUTTON_*, relative
   delta rx/ry (absolute position is derived by the server) */
static void tp_mouse(int state, int button, int rx, int ry) {
    xevent_t ev;
    memset(&ev, 0, sizeof(ev));
    if(_mouse_mode)
        ev.type = XEVT_MOUSE_TPAD;
    else
        ev.type = XEVT_MOUSE;

    ev.state = state;
    ev.value.mouse.relative = 1;
    ev.value.mouse.rx = rx;
    ev.value.mouse.ry = ry;
    ev.value.mouse.button = button;
    tp_send(&ev);
}

/* ---------------- first-contact tracking ---------------- */

/* Only the FIRST active contact is tracked; every other finger on the pad
   is ignored. `first` is the first tip-valid (confidence-accepted) contact
   of the frame, so multi-touch pads behave like a single-finger pad. A
   lift-and-reland never jumps the pointer: the new contact is re-anchored
   so its first delta is zero. */
static bool _first_down = false;
static int _fx = 0, _fy = 0;     /* current logical position */
static int _flx = 0, _fly = 0;   /* previous frame position */

static void tp_update_first(const touchpad_evt_t* evt) {
    int n = evt->contact_count;
    const touchpad_contact_t* f = NULL;
    bool conf_used = false;

    if (n > HID_TOUCHPAD_MAX_CONTACTS)
        n = HID_TOUCHPAD_MAX_CONTACTS;

    for (int i = 0; i < n; ++i) {
        if (evt->contacts[i].confidence)
            conf_used = true;
    }
    for (int i = 0; i < n; ++i) {
        const touchpad_contact_t* c = &evt->contacts[i];
        if (!c->tip)
            continue;
        if (conf_used && !c->confidence)
            continue; /* palm / not a real finger */
        f = c;
        break; /* only the first contact matters */
    }

    if (f == NULL) {
        _first_down = false;
        return;
    }
    if (!_first_down) {
        /* new contact: anchor so the first delta is zero */
        _first_down = true;
        _fx = _flx = f->x;
        _fy = _fly = f->y;
    }
    else {
        _flx = _fx;
        _fly = _fy;
        _fx = f->x;
        _fy = f->y;
    }
}

/* ---------------- gesture state machine ---------------- */

enum {
    TP_ST_IDLE = 0,   /* no tracked contact */
    TP_ST_WAIT,       /* first contact down, deciding click / drag / move */
    TP_ST_DRAG,       /* held still tap_time ms: LEFT down, motion drags */
    TP_ST_MOVE,       /* moved past tap_move: plain pointer move */
};

static int _state = TP_ST_IDLE;
static bool _pressed = false;   /* synthetic left DOWN emitted (drag) */
static uint64_t _down_ms = 0;   /* first-contact down time */
static int _down_x = 0, _down_y = 0; /* first-contact down position */
/* fractional accumulators */
static double _acc_x = 0, _acc_y = 0;
/* physical buttons */
static uint8_t _last_buttons = 0;
/* double-tap folding: a second click landing within this window, near the
   first click point, is swallowed so a double tap reports as one click */
#define TP_DOUBLE_CLICK_MS 300u
static uint64_t _last_click_ms = 0;
static int _last_click_x = 0, _last_click_y = 0;
/* stuck-release fallback: any state with no new data for this long is
   force-released, so a silent pad can never leave a button down */
#define TP_STUCK_MS 100u
static uint64_t _last_frame_ms = 0; /* last touchpad frame arrival time */

static void tp_reset_gesture(void) {
    _state = TP_ST_IDLE;
    _pressed = false;
    _acc_x = 0;
    _acc_y = 0;
    _first_down = false;
}

/* relative pointer move; state is MOVE or DRAG */
static void tp_pointer_move(int dx, int dy, int x_max, int y_max,
        int state, int button) {
    double px, py;
    int rx, ry;

    if (dx == 0 && dy == 0)
        return;
    if (x_max <= 0)
        x_max = 1000;
    if (y_max <= 0)
        y_max = 1000;
    px = (double)dx * _cfg.gain_x * (double)_scr_w / (double)x_max;
    py = (double)dy * _cfg.gain_y * (double)_scr_h / (double)y_max;
    _acc_x += px;
    _acc_y += py;
    rx = (int)_acc_x;
    ry = (int)_acc_y;
    if (rx == 0 && ry == 0)
        return;
    _acc_x -= rx;
    _acc_y -= ry;
    tp_mouse(state, button, rx, ry);
}

static void tp_do_click(void) {
    tp_mouse(MOUSE_STATE_DOWN, MOUSE_BUTTON_LEFT, 0, 0);
    tp_mouse(MOUSE_STATE_UP, MOUSE_BUTTON_LEFT, 0, 0);
}

static void tp_start_drag(void) {
    if (_pressed)
        return;
    _pressed = true;
    tp_mouse(MOUSE_STATE_DOWN, MOUSE_BUTTON_LEFT, 0, 0);
}

static void tp_end_drag(void) {
    if (!_pressed)
        return;
    _pressed = false;
    tp_mouse(MOUSE_STATE_UP, MOUSE_BUTTON_LEFT, 0, 0);
}

/* Timer-driven transitions, called from the main loop every poll tick:

   1. still-hold -> drag: a first contact that has stayed still for
      tap_time ms becomes a drag even if the pad stopped reporting frames.

   2. stuck-release: with no data for TP_STUCK_MS every pressed state is
      force-released (synthetic drag UP, physical-button UPs) and the
      machine returns to IDLE, so a silent pad can never leave a button
      down. A healthy stream of frames -- including byte-identical ones,
      which the host forwards for touchpads -- keeps this from firing
      while a finger is merely held still. */
static void tp_tick(void) {
    uint64_t now = kernel_tic_ms(0);

    if (_state == TP_ST_WAIT &&
            now - _down_ms >= (uint64_t)_cfg.tap_time) {
        _state = TP_ST_DRAG;
        tp_start_drag(); /* synthetic LEFT DOWN: held contact = drag */
        /* re-anchor: entering drag must not emit the pre-drag motion */
        _flx = _fx;
        _fly = _fy;
    }

    if (_last_frame_ms != 0 && now - _last_frame_ms >= (uint64_t)TP_STUCK_MS) {
        static const int btn_map[3] = {
            MOUSE_BUTTON_LEFT, MOUSE_BUTTON_RIGHT, MOUSE_BUTTON_MID
        };
        if (_state == TP_ST_DRAG)
            tp_end_drag(); /* lift-release of the synthetic drag */
        /* release any physical buttons still held: their lift frames were
           never delivered */
        for (int i = 0; i < 3; ++i) {
            if (_last_buttons & (1u << i))
                tp_mouse(MOUSE_STATE_UP, btn_map[i], 0, 0);
        }
        _last_buttons = 0;
        _pressed = false;
        _first_down = false;
        _state = TP_ST_IDLE;
        /* re-arm: while the pad stays silent the release runs at most once
           per TP_STUCK_MS instead of on every poll tick */
        _last_frame_ms = now;
    }
}

/* ---------------- frame processing ---------------- */

static void tp_handle_buttons(uint8_t buttons) {
    static const int btn_map[3] = {
        MOUSE_BUTTON_LEFT, MOUSE_BUTTON_RIGHT, MOUSE_BUTTON_MID
    };
    uint8_t diff = buttons ^ _last_buttons;

    for (int i = 0; i < 3; ++i) {
        if ((diff & (1u << i)) == 0)
            continue;
        if ((buttons & (1u << i)) != 0) {
            /* a physical click cancels any pending synthetic gesture */
            if (_state != TP_ST_IDLE)
                _state = TP_ST_IDLE;
            tp_mouse(MOUSE_STATE_DOWN, btn_map[i], 0, 0);
        }
        else {
            tp_mouse(MOUSE_STATE_UP, btn_map[i], 0, 0);
        }
    }
    _last_buttons = buttons;
}

static void tp_process_frame(const touchpad_evt_t* evt) {
    int x_max = evt->x_max > 0 ? evt->x_max : 0;
    int y_max = evt->y_max > 0 ? evt->y_max : 0;
    int tap2;

    _last_frame_ms = kernel_tic_ms(0);
    tp_update_first(evt);
    tp_handle_buttons(evt->buttons);

    /* physical buttons held: plain drag of the first contact, no click /
       drag / move arbitration (the button edges were emitted above) */
    if (evt->buttons != 0) {
        int held = (evt->buttons & 0x1) ? MOUSE_BUTTON_LEFT :
                ((evt->buttons & 0x2) ? MOUSE_BUTTON_RIGHT : MOUSE_BUTTON_MID);
        if (_first_down) {
            tp_pointer_move(_fx - _flx, _fy - _fly,
                    x_max, y_max, MOUSE_STATE_DRAG, held);
        }
        _state = TP_ST_IDLE;
        return;
    }

    switch (_state) {
    case TP_ST_IDLE:
        if (_first_down) {
            _state = TP_ST_WAIT;
            _down_ms = kernel_tic_ms(0);
            _down_x = _fx;
            _down_y = _fy;
        }
        break;

    case TP_ST_WAIT:
        if (!_first_down) {
            /* released while still deciding: < tap_time and still = click,
               otherwise a held-then-released press */
            uint64_t now = kernel_tic_ms(0);
            int dx = _fx - _down_x;
            int dy = _fy - _down_y;
            int move2 = dx * dx + dy * dy;
            tap2 = tp_tap_move(x_max) * tp_tap_move(x_max);
            if ((now - _down_ms) < (uint64_t)_cfg.tap_time &&
                    move2 <= tap2) {
                /* click, with double-tap folding */
                int tol = tp_tap_move(x_max) * 2;
                if (_last_click_ms != 0 &&
                        (now - _last_click_ms) <= (uint64_t)TP_DOUBLE_CLICK_MS &&
                        abs(_down_x - _last_click_x) <= tol &&
                        abs(_down_y - _last_click_y) <= tol) {
                    _last_click_ms = 0; /* swallow the repeat */
                }
                else {
                    tp_do_click();
                    _last_click_ms = now;
                    _last_click_x = _down_x;
                    _last_click_y = _down_y;
                }
            }
            else if (_cfg.tap_enable) {
                /* held past the tap window without intermediate frames:
                   press+release so the button state stays balanced */
                tp_do_click();
            }
            _state = TP_ST_IDLE;
            break;
        }
        if (!_cfg.tap_enable) {
            /* tapping disabled: a still contact just moves the pointer */
            _state = TP_ST_MOVE;
            break;
        }
        {
            int dx = _fx - _down_x;
            int dy = _fy - _down_y;
            int m2 = tp_move_threshold(x_max) * tp_move_threshold(x_max);
            if (dx * dx + dy * dy > m2) {
                /* moved past the debounce threshold: pointer mode (no
                   button) */
                _state = TP_ST_MOVE;
                _flx = _fx; /* re-anchor: no delta jump on mode switch */
                _fly = _fy;
            }
        }
        break;

    case TP_ST_DRAG:
        if (!_first_down) {
            tp_end_drag(); /* lift releases the drag */
            _state = TP_ST_IDLE;
            break;
        }
        tp_pointer_move(_fx - _flx, _fy - _fly,
                x_max, y_max, MOUSE_STATE_DRAG, MOUSE_BUTTON_LEFT);
        break;

    case TP_ST_MOVE:
        if (!_first_down) {
            _state = TP_ST_IDLE;
            break;
        }
        tp_pointer_move(_fx - _flx, _fy - _fly,
                x_max, y_max, MOUSE_STATE_MOVE, MOUSE_BUTTON_NONE);
        break;
    }
}

/* ================= main ================= */

static void usage(const char* prog) {
    printf("Usage: %s [-c config_file] [-m] [touch_dev]\n", prog);
    printf("  default   single-touch absolute mode (/dev/touch0)\n");
    printf("  -m        Precision Touchpad (PTP) mode: reads first-contact\n");
    printf("            frames from /dev/touchpad0: still 300ms = drag,\n");
    printf("            short still tap = click, moving = pointer move,\n");
    printf("            plus physical buttons\n");
    printf("  -c        config file (default /etc/x/xtouch.json, or\n");
    printf("            /etc/x/xtouchpad.json with -m)\n");
}

int main(int argc, char** argv) {
    const char* config = NULL;
    const char* touch_dev = NULL;
    int opt;

    while((opt = getopt(argc, argv, "c:mh")) != -1) {
        switch(opt) {
        case 'c':
            config = optarg;
            break;
        case 'm':
            _mouse_mode = true;
            break;
        case 'h':
        default:
            usage(argv[0]);
            return opt == 'h' ? 0 : -1;
        }
    }

    if(optind < argc)
        touch_dev = argv[optind];
    if(config == NULL)
        config = _mouse_mode ? TP_DEFAULT_CONFIG : _default_config;
    if(touch_dev == NULL)
        touch_dev = _mouse_mode ? TP_DEFAULT_DEV : _default_touch0;

    if(_mouse_mode)
        tp_load_config(config);
    else
        read_config(config);

    int fd = -1;
    while(true) {
        fd = open(touch_dev, O_RDONLY);
        if(fd > 0)
            break;
        usleep(100000);
    }

    while(true) {
        _x_pid = dev_get_pid("/dev/x");
        if(_x_pid > 0) {
            xscreen_info_t scr;
            x_screen_info(&scr, 0);
            if(scr.size.w > 0)
                _scr_w = scr.size.w;
            if(scr.size.h > 0)
                _scr_h = scr.size.h;
            break;
        }
        usleep(100000);
    }

    if(_mouse_mode) {
        tp_reset_gesture();
        /* non-blocking read: the poll loop also drives the timer-based
           still-hold -> drag transition even when the pad sends nothing */
        int fl = fcntl(fd, F_GETFL, 0);
        if(fl >= 0)
            (void)fcntl(fd, F_SETFL, fl | O_NONBLOCK);
        while(true) {
            touchpad_evt_t evt;
            ensure_x_server();
            if(read(fd, &evt, sizeof(evt)) == (int)sizeof(evt)) {
                tp_process_frame(&evt);
            }
            tp_tick();
            usleep(2000);
        }
    }

    uint16_t prev_ev = 0;
    while(true) {
        int8_t buf[6];
        memset(buf, 0, 6);
        ensure_x_server();
        /*
         * Blocking read: the device node has no data until the driver has a
         * position for us, so the read itself is the wait.  This loop used to
         * poll it and then usleep(3000) on every pass, which added up to 3 ms
         * of pure delay to every touch sample (and did so whether or not an
         * event was waiting, because the sleep was unconditional).
         */
        if(read(fd, buf, 6) == 6) {
            uint16_t* mv = (uint16_t*)buf;
            if(mv[0] == 0 && prev_ev == 0)
                continue;
            prev_ev = mv[0];
            int16_t xraw = mv[1], yraw = mv[2];
            if(_xtouch.xy_switch) {
                xraw = mv[2];
                yraw = mv[1];
            }

            int16_t tx = xraw - _xtouch.x_offset;
            int16_t ty = yraw - _xtouch.y_offset;
            tx = tx < 0 ? 0 : tx;
            ty = ty < 0 ? 0 : ty;

            input(mv[0], tx, ty);
        }
    }

    close(fd);
    return 0;
}