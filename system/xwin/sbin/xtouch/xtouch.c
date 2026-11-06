#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <tinyjson/tinyjson.h>
#include <ewoksys/proto.h>
#include <ewoksys/core.h>
#include <x/xcntl.h>
#include <x/xevent.h>
#include <x/xwin.h>
#include <ewoksys/vfs.h>
#include <mouse/mouse.h>

static int _x_pid = -1;
static int _scr_w = 0;
static int _scr_h = 0;

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

/* -m: mouse-relative mode. Touch motion moves the cursor by relative deltas
   instead of teleporting it to the touched point (touchpad style: touching
   down does not move the cursor, only the finger's travel does). A contact
   does not press the left button right away: it stays a plain move until the
   finger has been held still for XT_HOLD_MS, so a slide never turns into a
   drag. Only a finger kept still for that whole window is then pressed, and
   further travel becomes a drag; once a contact has slid it is locked to pure
   cursor motion for its whole lifetime, so pausing mid-slide never presses
   the button either. Release never carries motion: the lift-off sample's
   coordinates are not trustworthy, so every event on release is emitted in
   place. Before any press there are two cases - a slide (travel past
   XT_TAP_SLOP) ignores the button entirely (no DOWN, no UP), while a
   stationary tap synthesizes a down+up click in place, so every press is
   matched by a release and the server never sees a dangling button. */
static bool _mouse_mode = false;
#define XT_HOLD_MS 500
/* A finger "held still" for the long-press still jitters on the panel, so this
   slop must be wide enough that hold-jitter never counts as a slide, while a
   deliberate slide (well past it) does. 20 screen px is a safe middle ground:
   press-and-hold jitter stays under it, real drags fly past it. */
#define XT_TAP_SLOP 20
static bool _in_contact = false;  /* finger currently down */
static int _prev_state = 0;       /* previous report state, for edge detection */
static uint64_t _press_time = 0;  /* kernel tic when the current contact began */
static bool _press_sent = false;  /* left-button DOWN already reported */
static bool _moved = false;       /* current contact travelled past the tap slop */
static int _down_x = 0, _down_y = 0; /* mapped press point of the current contact */
static int _last_mx = 0, _last_my = 0; /* last mapped touch position (relative anchor) */

static void usage(const char* prog) {
    printf("Usage: %s [-c config_file] [-m] [touch_dev]\n", prog);
    printf("  -m  mouse-relative mode: touch moves the cursor relatively,\n");
    printf("      move by default, hold %dms then move to drag\n", XT_HOLD_MS);
}

/* one relative pointer event: rx/ry carry the motion, state is MOVE/DRAG/
   DOWN/UP. xserverd applies the delta to the cursor first, then routes the
   button state at the resulting position, so a single event is enough. */
static void mouse_rel_event(int32_t rx, int32_t ry, int32_t state, int32_t button) {
    xevent_t ev;
    memset(&ev, 0, sizeof(xevent_t));
    ev.type = XEVT_MOUSE;
    ev.state = state;
    ev.value.mouse.relative = 1;
    ev.value.mouse.rx = rx;
    ev.value.mouse.ry = ry;
    ev.value.mouse.button = button;

    proto_t in;
    PF->init(&in)->add(&in, &ev, sizeof(xevent_t));
    dev_cntl_by_pid(_x_pid, X_DCNTL_INPUT, &in, NULL);
    PF->clear(&in);
}

/* map raw touch coordinates to screen coordinates (same mapping as the
   absolute path in input()) */
static void map_touch_pos(int16_t tx, int16_t ty, int* x, int* y) {
    if(_xtouch.x_rev)
        *x = _scr_w - (tx*_scr_w / _xtouch.x_max);
    else
        *x = (tx*_scr_w / _xtouch.x_max);

    if(_xtouch.y_rev)
        *y = (ty*_scr_h / _xtouch.y_max);
    else
        *y = _scr_h - (ty*_scr_h / _xtouch.y_max);
}

/* consume the travel since the last sample as a relative delta */
static int mouse_rel_delta(int nx, int ny, int* rx, int* ry) {
    *rx = nx - _last_mx;
    *ry = ny - _last_my;
    _last_mx = nx;
    _last_my = ny;
    return (*rx != 0 || *ry != 0);
}

static void input_mouse(int state, int nx, int ny) {
    int rx, ry;
    bool rising = (state == 1 && _prev_state == 0);
    _prev_state = state;

    if(state == 1) { //contact
        /* A rising edge - or any leftover inconsistent state - always begins a
           fresh contact and re-anchors here. Re-anchoring is what stops the
           cursor from jumping: without it the first sample of a new touch gets
           diffed against the previous gesture's last point and flings the
           cursor across the screen. */
        if(rising || !_in_contact) {
            _in_contact = true;
            _press_time = kernel_tic_ms(0);
            _press_sent = false;
            _moved = false;
            _down_x = nx;
            _down_y = ny;
            _last_mx = nx;
            _last_my = ny;
            return;
        }
        int has_delta = mouse_rel_delta(nx, ny, &rx, &ry);
        //classify as a slide once it strays past the slop from the press point
        if(!_moved &&
                (abs(nx - _down_x) > XT_TAP_SLOP || abs(ny - _down_y) > XT_TAP_SLOP))
            _moved = true;
        if(!has_delta)
            return;
        //held long enough -> drag; otherwise just move the cursor
        if(_press_sent)
            mouse_rel_event(rx, ry, MOUSE_STATE_DRAG, MOUSE_BUTTON_LEFT);
        else
            mouse_rel_event(rx, ry, MOUSE_STATE_MOVE, 0);
    }
    else { //release
        if(!_in_contact)
            return;
        _in_contact = false;
        /* Release never carries motion: some panels report (0,0) or a stale
           point on lift-off, and diffing that flings the cursor on every
           release. All real travel was already emitted by the contact samples,
           so here we only send the button transition, in place. */
        if(_press_sent) {
            //end the long-press drag with a clean button-up
            mouse_rel_event(0, 0, MOUSE_STATE_UP, MOUSE_BUTTON_LEFT);
        }
        else if(_moved) {
            //a slide that never reached the long-press is pure cursor motion:
            //ignore the button entirely - no DOWN was sent, so send no UP
        }
        else {
            //stationary tap: click in place (down+up), never a jump
            mouse_rel_event(0, 0, MOUSE_STATE_DOWN, MOUSE_BUTTON_LEFT);
            mouse_rel_event(0, 0, MOUSE_STATE_UP, MOUSE_BUTTON_LEFT);
        }
        _press_sent = false;
        _moved = false;
    }
}

/* hold timer tick: a stationary held finger generates no read events, so the
   deferred press is armed from the main loop once the hold expires. The
   cursor is already where the finger settled, so press in place (rx=ry=0).
   A contact that has already turned into a slide stays pure cursor motion for
   its whole lifetime, so a mid-slide pause never re-arms the long-press and
   the rest of the gesture cannot become a drag. */
static void input_mouse_tick(void) {
    if(!_in_contact || _press_sent || _moved)
        return;
    if(kernel_tic_ms(0) - _press_time < XT_HOLD_MS)
        return;
    mouse_rel_event(0, 0, MOUSE_STATE_DOWN, MOUSE_BUTTON_LEFT);
    _press_sent = true;
}

static void input(uint16_t state, int16_t tx, int16_t ty) {
    //slog("xtouch raw: %d, %d\n", tx, ty);
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

int main(int argc, char** argv) {
    const char* config = _default_config;
    const char* touch_dev = "/dev/touch0";
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

    _x_pid = -1;
    read_config(config);

    int fd = -1;
    while(true) {
        /* mouse mode polls for the deferred long-press, so the device must
           not block the loop; absolute mode keeps the original blocking read */
        fd = open(touch_dev, _mouse_mode ? (O_RDONLY | O_NONBLOCK) : O_RDONLY);
        if(fd > 0)
            break;
        usleep(100000);
    }

    while(true) {
        _x_pid = dev_get_pid("/dev/x");
        if(_x_pid > 0) {
            xscreen_info_t scr;
            x_screen_info(&scr, 0);
            _scr_w = scr.size.w;
            _scr_h = scr.size.h;
            if(_mouse_mode) {
                /* one-time absolute placement: the relative cursor starts at
                   the screen center instead of wherever it happened to be */
                _last_mx = _scr_w / 2;
                _last_my = _scr_h / 2;
                xevent_t ev;
                memset(&ev, 0, sizeof(xevent_t));
                ev.type = XEVT_MOUSE;
                ev.state = MOUSE_STATE_MOVE;
                ev.value.mouse.relative = 0;
                ev.value.mouse.x = _last_mx;
                ev.value.mouse.y = _last_my;
                proto_t in;
                PF->init(&in)->add(&in, &ev, sizeof(xevent_t));
                dev_cntl_by_pid(_x_pid, X_DCNTL_INPUT, &in, NULL);
                PF->clear(&in);
            }
            break;
        }
        usleep(100000);
    }

    uint16_t prev_ev = 0;
    while(true) {
        int8_t buf[6];
        memset(buf, 0, 6);
        if(read(fd, buf, 6) == 6) {
            uint16_t* mv = (uint16_t*)buf;
            if(mv[0] == 0 && prev_ev == 0)
                continue;
            prev_ev = mv[0];

            uint16_t xraw = mv[1];
            uint16_t yraw = mv[2];
            if(_xtouch.xy_switch) {
                xraw = mv[2];
                yraw = mv[1];
            }

            int16_t tx = xraw - _xtouch.x_offset;
            int16_t ty = yraw - _xtouch.y_offset;
            //slog("xtouch: tx:%d rawx:%d xoff:%d xmax:%d\n", tx, xraw, _xtouch.x_offset, _xtouch.x_max);
            //slog("xtouch: ty:%d rawy:%d yoff:%d ymax:%d\n\n", ty, yraw, _xtouch.y_offset, _xtouch.y_max);
            tx = tx < 0 ? 0 : tx;
            ty = ty < 0 ? 0 : ty;

            if(_mouse_mode) {
                int nx, ny;
                map_touch_pos(tx, ty, &nx, &ny);
                input_mouse(mv[0], nx, ny);
            }
            else {
                input(mv[0], tx, ty);
            }
        }
        if(_mouse_mode)
            input_mouse_tick();
        usleep(3000);
    }

    close(fd);
    return 0;
}
