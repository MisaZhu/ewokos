/*mouse/touch and IME input handling: pointer tracking, window hit
  testing, drag move/resize gestures and event routing to windows*/
#include <stdlib.h>
#include <string.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/klog.h>
#include "xinput.h"
#include "xwin.h"
#include "xrepaint.h" //hide_cursor

static int win_frame_pos_at(x_t* x, xwin_t* win, int32_t px, int32_t py) {
    /*a decoration-less window (NO_FRAME|NO_TITLE) has empty hit areas, so the
      rect tests below simply match nothing and return -1. A NO_FRAME window
      that keeps its title still exposes its title/close/min/max areas (its
      resize area is empty), so it hit-tests like a normal window here.*/
    int res = -1;
    if(check_in_rect(px, py, &win->r_close))
        res = FRAME_R_CLOSE;
    else if(check_in_rect(px, py, &win->r_min))
        res = FRAME_R_MIN;
    else if(check_in_rect(px, py, &win->r_max))
        res = FRAME_R_MAX;
    else if(check_in_rect(px, py, &win->r_title))
        res = FRAME_R_TITLE;
    else if(check_in_rect(px, py, &win->r_resize))
        res = FRAME_R_RESIZE;
    return res;
}

/*topmost visible window under an arbitrary point; win_frame_pos gets the
  frame area hit (or -1 for the workspace part)*/
static xwin_t* get_owner_at(x_t* x, int32_t px, int32_t py, int* win_frame_pos) {
    xwin_t* win = x->win_tail;
    if(win_frame_pos != NULL)
        *win_frame_pos = -1;

    while(win != NULL) {
        if(win->xinfo == NULL ||
                !win->xinfo->visible ||
                (win->xinfo->style & XWIN_STYLE_LAZY) != 0 ||
                win->xinfo->display_index != x->current_display) {
            win = win->prev;
            continue;
        }
        int pos = win_frame_pos_at(x, win, px, py);
        if(pos >= 0) {
            if(win_frame_pos != NULL)
                *win_frame_pos = pos;
            return win;
        }
        if(check_in_rect(px, py, &win->xinfo->wsr))
            return win;
        win = win->prev;
    }
    return NULL;
}

xwin_t* get_mouse_owner(x_t* x, int* win_frame_pos) {
    return get_owner_at(x, x->cursor.cpos.x, x->cursor.cpos.y, win_frame_pos);
}

int x_cursor_set_busy(x_t* x, bool busy) {
    if(x->mouse_state.busy == busy)
        return 0;

    /*only erase when a cursor image is actually on screen (saved != NULL):
      with the pointer hidden (touch input) hide_cursor would instead capture
      a background band at the stale old_pos, faking an on-screen cursor whose
      later "restore" would smear an outdated scene patch onto the display*/
    if(x->cursor.saved != NULL) {
        hide_cursor(x);
        graph_free(x->cursor.saved);
        x->cursor.saved = NULL;
    }

    x->mouse_state.busy = busy;
    if(busy && x->cursor.img_busy != NULL) {
        x->cursor.size.w = x->cursor.img_busy->w;
        x->cursor.size.h = x->cursor.img_busy->h;
        x->cursor.offset.x = x->cursor.offset_busy.x;
        x->cursor.offset.y = x->cursor.offset_busy.y;
    }
    else if(x->cursor.img != NULL) {
        x->cursor.size.w = x->cursor.img->w;
        x->cursor.size.h = x->cursor.img->h;
        x->cursor.offset.x = x->cursor.offset_normal.x;
        x->cursor.offset.y = x->cursor.offset_normal.y;
    }
    x_repaint_req(x, x->current_display);
    return 0;
}

static void mouse_cxy(x_t* x, uint32_t display_index, int32_t rx, int32_t ry) {
    x_display_t* display = &x->displays[display_index];
    x->cursor.cpos.x += rx;
    x->cursor.cpos.y += ry;

    if(x->cursor.cpos.x < 0)
        x->cursor.cpos.x = 0;

    if(x->cursor.cpos.x > (int32_t)display->g->w)
        x->cursor.cpos.x = display->g->w;

    if(x->cursor.cpos.y < 0)
        x->cursor.cpos.y = 0;

    if(x->cursor.cpos.y >= (int32_t)display->g->h)
        x->cursor.cpos.y = display->g->h;
}

static void mouse_xwin_handle(x_t* x, xwin_t* win, int pos, xevent_t* ev) {
    if(ev->state ==  MOUSE_STATE_DOWN) {
        if(win != x->win_tail) {
            xwin_top(x, win);
        }
        else {
            try_focus(x, win);
        }
        
        if(pos == FRAME_R_TITLE) {//window title 
            x->current.win_drag = win;
            x->current.old_pos.x = x->cursor.cpos.x;
            x->current.old_pos.y = x->cursor.cpos.y;
            x->current.drag_state = X_win_DRAG_MOVE;
        }
        else if(pos == FRAME_R_RESIZE) {//window resize
            x->current.win_drag = win;
            x->current.old_pos.x = x->cursor.cpos.x;
            x->current.old_pos.y = x->cursor.cpos.y;
            x->current.drag_state = X_win_DRAG_RESIZE;
        }
        else if(pos == FRAME_R_MAX || pos == FRAME_R_CLOSE) {
            return;
        }
        else if(win_decoration_less(win)) {
            /*a decoration-less window (sprite/statusbar) has no title to grab,
              so a press anywhere in it used to drag the whole window. Fixed-size
              borderless clients (games and video surfaces) have no meaningful
              drag operation; route their presses to the client instead.*/
            if(win->xinfo->state == XWIN_STATE_MAX ||
               (win->xinfo->style & XWIN_STYLE_NO_RESIZE) != 0) {
                x->current.mouse_grab = win;
            } else {
                x->current.old_pos.x = x->cursor.cpos.x;
                x->current.old_pos.y = x->cursor.cpos.y;
                x->current.win_drag = win;
            }
        }
        else {
            /*plain client-area press: grab the pointer for this window until
              release, so a drag that runs past the window edge (or off every
              window) still delivers its DRAG/UP events to the gesture owner
              instead of dying on whatever happens to be under the cursor.*/
            x->current.mouse_grab = win;
        }
    }
    else if(ev->state ==  MOUSE_STATE_DRAG) {
        if(win->xinfo->state != XWIN_STATE_MAX) {
            if(pos == FRAME_R_TITLE) {//window title 
                x->current.old_pos.x = x->cursor.cpos.x;
                x->current.old_pos.y = x->cursor.cpos.y;
                x->current.drag_state = X_win_DRAG_MOVE;
            }
            else if(pos == FRAME_R_RESIZE) {//window resize
                x->current.old_pos.x = x->cursor.cpos.x;
                x->current.old_pos.y = x->cursor.cpos.y;
                x->current.drag_state = X_win_DRAG_RESIZE;
            }
        }
    }
    else if(ev->state == MOUSE_STATE_UP) {
        if(pos == FRAME_R_RESIZE) {//window resize
            /*still clear the drag state before bailing out: returning early
              here used to skip the win_drag/drag_state reset at the end of this
              branch, so a button release over the resize corner could leave the
              drag latched and its overlay outline stuck on screen*/
            x->current.win_drag = NULL;
            x->current.drag_state = 0;
            return;
        }

        if(x->current.win_drag == win &&
                x->current.drag_state != 0 &&
                win->xinfo->state != XWIN_STATE_MAX) {
            ev->type = XEVT_WIN;
            ev->value.window.v0 =  x->current.pos_delta.x;
            ev->value.window.v1 =  x->current.pos_delta.y;
            if(x->current.drag_state == X_win_DRAG_RESIZE) {
                if(x->current.pos_delta.x != 0 ||
                    x->current.pos_delta.y != 0 ) {
                    ev->value.window.event = XEVT_WIN_RESIZE;
                }
                else {
                    /*a press+release on the resize corner with no movement must
                      not reach the client. window.event still aliases the mouse
                      event's "relative" flag here, which reads 1 == XEVT_WIN_CLOSE
                      for a touchpad and would otherwise close the window.*/
                    ev->value.window.event = XEVT_WIN_NONE;
                }
            }
            else if(x->current.drag_state == X_win_DRAG_MOVE) {
                ev->value.window.event = XEVT_WIN_MOVE;
            }
            x->current.pos_delta.x = 0;
            x->current.pos_delta.y = 0;
        }
        else if(!x->current.mt_no_click &&
                abs_32(ev->value.mouse.from_x - ev->value.mouse.x) < (uint32_t)(6 + x->current.mt_click_slack) &&
                abs_32(ev->value.mouse.from_y - ev->value.mouse.y) < (uint32_t)(6 + x->current.mt_click_slack)) {
            x_push_event(x, win, ev);
            ev->state = MOUSE_STATE_CLICK;
        }

        if(ev->state == MOUSE_STATE_CLICK) {
            if(pos == FRAME_R_CLOSE) { //window close
                ev->type = XEVT_WIN;
                ev->value.window.event = XEVT_WIN_CLOSE;
            }
            else if(pos == FRAME_R_MAX) {
                ev->type = XEVT_WIN;
                ev->value.window.event = XEVT_WIN_MAX;
            }
        }
        x->current.win_drag = NULL;
        x->current.drag_state = 0;
        x->current.mouse_grab = NULL;
    }

    if(x->current.win_drag == win && x->current.drag_state != 0) {
        int mrx = x->cursor.cpos.x - x->current.old_pos.x;
        int mry = x->cursor.cpos.y - x->current.old_pos.y;
        if(abs(mrx) > 3 || abs(mry) > 3) {
            x->current.pos_delta.x = mrx;
            x->current.pos_delta.y = mry;
        }
        /*the drag frame is an overlay now: only its old and new outline
          rects are repainted, the scene underneath stays untouched*/
        x_repaint_req(x, x->current_display);
        return; //drag win frame, don't push xwin event.
    }

    if(ev->type == XEVT_WIN && ev->value.window.event == XEVT_WIN_NONE)
        return;
    x_push_event(x, win, ev);
}

static void cursor_safe(x_t* x, x_display_t* display) {
    int margin_x = (x->cursor.size.w - x->cursor.offset.x) / 4;
    int margin_y = (x->cursor.size.h - x->cursor.offset.y) / 4;

    if(x->cursor.cpos.x < x->cursor.offset.x)
        x->cursor.cpos.x = x->cursor.offset.x;
    else if(x->cursor.cpos.x > (display->g->w - margin_x))
        x->cursor.cpos.x = display->g->w - margin_x;

    if(x->cursor.cpos.y < x->cursor.offset.y)
        x->cursor.cpos.y = x->cursor.offset.y;
    else if(x->cursor.cpos.y > (display->g->h - margin_y))
        x->cursor.cpos.y = display->g->h - margin_y;
}

static int mouse_handle(x_t* x, xevent_t* ev) {
    if(x->mouse_state.state == MOUSE_STATE_NONE && ev->state == MOUSE_STATE_UP) {
        /*per-event flags reset on EVERY return path, or a cull's no-click
          leaks into the next real tap and eats its CLICK*/
        x->current.mt_no_click = false;
        x->current.mt_click_slack = 0;
        return 0;
    }

    if(ev->value.mouse.relative != 0) {
        mouse_cxy(x, x->current_display, ev->value.mouse.rx, ev->value.mouse.ry);
        ev->value.mouse.x = x->cursor.cpos.x;
        ev->value.mouse.y = x->cursor.cpos.y;
    }
    else {
        x->cursor.cpos.x = ev->value.mouse.x;
        x->cursor.cpos.y = ev->value.mouse.y;
    }

    ev->value.mouse.rx = ev->value.mouse.x - x->mouse_state.last_pos.x;
    ev->value.mouse.ry = ev->value.mouse.y - x->mouse_state.last_pos.y;
    x->mouse_state.last_pos.x = ev->value.mouse.x;
    x->mouse_state.last_pos.y = ev->value.mouse.y;

    x_display_t *display = &x->displays[x->current_display];
    display->cursor_task = true;
    cursor_safe(x, display);
    if(ev->state ==  MOUSE_STATE_DOWN) {
        x->cursor.down = true;
        if(x->mouse_state.state == 0) {
            x->mouse_state.state = MOUSE_STATE_DOWN;
            x->mouse_state.down_pos.x = ev->value.mouse.x;
            x->mouse_state.down_pos.y = ev->value.mouse.y;
        }
        else if(abs_32(ev->value.mouse.rx) > 3 ||
                abs_32(ev->value.mouse.ry) > 3 ||
                    x->mouse_state.state == MOUSE_STATE_DRAG) {
            /*repeated DOWN while the button is held: promote to DRAG once
              the pointer travels. Compare the rx/ry deltas computed above
              from the PREVIOUS position - last_pos already holds the new
              position here, so comparing it against the event would always
              be zero and a touch stream of repeated DOWNs would never drag.*/
            x->mouse_state.state = MOUSE_STATE_DRAG;
            ev->state = MOUSE_STATE_DRAG;
            ev->value.mouse.from_x = x->mouse_state.down_pos.x;
            ev->value.mouse.from_y = x->mouse_state.down_pos.y;
        }
    }
    else if(ev->state ==  MOUSE_STATE_UP) {
        x->cursor.down = false;
        x->mouse_state.state = MOUSE_STATE_NONE;
        ev->value.mouse.from_x = x->mouse_state.down_pos.x;
        ev->value.mouse.from_y = x->mouse_state.down_pos.y;
    }

    /*persistent popup grab outranks every other routing rule: while a window
      holds it, each mouse event is pushed to that window raw and the handler
      returns - no frame hit-test, no win_drag/mouse_grab arming, no try_focus.
      Qt owns the menu from here: it takes the global-coordinate stream on the
      grabbing (top-level popup) window, redirects it to the topmost popup via
      mapFromGlobal and dismisses the whole cascade on a press that lands
      outside it. The owner releases the grab explicitly when the last popup
      closes; if it was deleted or hidden without releasing, drop the stale
      grab here and fall through to normal hit-testing for this event.*/
    if(x->current.popup_grab != NULL) {
        xwin_t* gwin = x->current.popup_grab;
        if(gwin->xinfo == NULL || !gwin->xinfo->visible) {
            x->current.popup_grab = NULL;
        }
        else {
            x_cursor_set_busy(x, gwin->busy);
            x_push_event(x, gwin, ev);
            x->current.mt_no_click = false;
            x->current.mt_click_slack = 0;
            if(x_cursor_redraw_now(x, x->current_display))
                display->cursor_task = false;
            return 0;
        }
    }

    int pos = -1;
    xwin_t* win = NULL;
    if(x->current.win_drag != NULL)
        win = x->current.win_drag;
    else if(x->current.mouse_grab != NULL) {
        /*the grab owner may have been hidden meanwhile (the XIM put down
          by a keyboard attach): a stale grab would keep routing every tap
          to the invisible window. Drop it and hit-test this event fresh,
          the same rule the popup grab above applies.*/
        if(x->current.mouse_grab->xinfo == NULL ||
                !x->current.mouse_grab->xinfo->visible) {
            x->current.mouse_grab = NULL;
            /*the in-flight press belonged to the hidden window: its lift
              must not promote to a CLICK on whatever is underneath*/
            if(ev->state == MOUSE_STATE_UP)
                x->current.mt_no_click = true;
        }
        else
            win = x->current.mouse_grab;
    }
    if(win == NULL)
        win = get_mouse_owner(x, &pos);

    if(win != NULL) {
        x_cursor_set_busy(x, win->busy);
        mouse_xwin_handle(x, win, pos, ev);
    }
    else {
        x_cursor_set_busy(x, false);
        if(ev->state ==  MOUSE_STATE_DOWN)
            x_unfocus(x);
    }
    /*the stale-grab cancel is per-event: never let it leak into the next
      tap's UP->CLICK promotion*/
    x->current.mt_no_click = false;
    x->current.mt_click_slack = 0;

    /*redraw the cursor right away (cheap composite + non-blocking flush)
      so the pointer tracks at event rate; waiting for the next paced
      frame would show every move a frame period late. When it is skipped
      (daemon mid-push / frame flush in flight) cursor_task stays set and
      the frame path draws it instead.*/
    if(x_cursor_redraw_now(x, x->current_display))
        display->cursor_task = false;

    return 0;
}

/*on-demand MT pipeline counters for the "stat" devcmd: no logging, just
  running totals read back when asked. They separate the three failure
  domains - frames never arriving (goodixd/xtouch), arriving but dropped
  here (no gesture owner), or delivered but not cleared by the client.*/
typedef struct {
    uint32_t frames;      /*XEVT_TOUCH_MT frames received*/
    uint32_t rel_only;    /*frames carrying only RELEASE edges*/
    uint32_t delivered;   /*frames pushed to a window*/
    uint32_t dropped;     /*frames with no owner at all*/
    int last_n;           /*entry count of the most recent frame*/
    char last_edges[64];  /*its entries as id+state, e.g. "0P 1R"*/
} mt_diag_t;
static mt_diag_t _mt_diag;

void x_mt_diag_stats(uint32_t* frames, uint32_t* rel_only,
        uint32_t* delivered, uint32_t* dropped,
        int* last_n, char* last_edges, size_t edges_size) {
    if(frames != NULL) *frames = _mt_diag.frames;
    if(rel_only != NULL) *rel_only = _mt_diag.rel_only;
    if(delivered != NULL) *delivered = _mt_diag.delivered;
    if(dropped != NULL) *dropped = _mt_diag.dropped;
    if(last_n != NULL) *last_n = _mt_diag.last_n;
    if(last_edges != NULL && edges_size > 0) {
        strncpy(last_edges, _mt_diag.last_edges, edges_size - 1);
        last_edges[edges_size - 1] = '\0';
    }
}

/*a lone finger starts dragging only after this hold, the same gate the
  legacy XEVT_TOUCH stream applies in xtouch: the jitter of a quick tap must
  not reach the client as drag travel*/
#define MT_DRAG_HOLD_MS 100

/*a touch tap tolerates more travel than the 6px mouse click threshold:
  finger jitter and roll-off routinely exceed it, which used to eat the
  keystroke. Added to the promotion distance of MT-synthesized UPs only.*/
#define MT_CLICK_SLACK 16

/*a second landing finger is a drag gesture only once the contact midpoint
  actually travels this far; a brief stationary overlap is typing rollover
  and must keep tapping. The vkey client engages its own two-finger drag at
  the same midpoint travel, so both sides classify the gesture alike.*/
#define MT_MULTI_SLOP 12

/*one synthetic left-button mouse event for the single-finger touch
  compatibility path: absolute position, routed exactly like the legacy
  XEVT_TOUCH stream (DOWN once, explicit DRAGs while it travels - mouse_handle
  does not promote repeated DOWNs - and the click derived from the down/up
  distance)*/
static void mt_synth_mouse(x_t* x, int32_t state, int32_t px, int32_t py) {
    xevent_t mev;
    memset(&mev, 0, sizeof(mev));
    mev.type = XEVT_MOUSE;
    mev.state = state;
    mev.value.mouse.relative = 0;
    mev.value.mouse.x = px;
    mev.value.mouse.y = py;
    mev.value.mouse.button = MOUSE_BUTTON_LEFT;
    mouse_handle(x, &mev);
}

/*midpoint of all active contacts from their last edge positions; returns
  the contact count (0 leaves cx/cy untouched)*/
static int mt_midpoint(x_t* x, int32_t* cx, int32_t* cy) {
    int32_t sx = 0, sy = 0, n = 0;
    for(int i = 0; i < X_TOUCH_MAX; i++) {
        if(x->current.mt_active_mask & (1u << i)) {
            sx += x->current.mt_last_x[i];
            sy += x->current.mt_last_y[i];
            n++;
        }
    }
    if(n > 0) {
        *cx = sx / n;
        *cy = sy / n;
    }
    return n;
}

/*multi-touch contact frames (XEVT_TOUCH_MT): the raw frame is delivered to
  the window the gesture belongs to, and a lone finger additionally drives
  the classic mouse semantics so existing clients keep working untouched.

  - one active contact, no multi session: synthesize LEFT DOWN on the press,
    DRAG while it travels and UP on release, and deliver the raw frame to
    the same window the mouse path grabbed;
  - second finger lands over a lone press: the gesture stays PENDING - fast
    typing overlaps fingers (rollover), so the press is not cancelled until
    the contact midpoint actually travels (a real two-finger drag: cancel
    the press as a plain UP that never becomes a CLICK, then only raw frames
    flow). If the pressing finger lifts first it taps normally and the
    remaining finger takes over the synthetic press;
  - every finger lifted: end the session and drop the grab.*/
static void touch_mt_handle(x_t* x, xevent_t* ev) {
    int count = ev->state;
    int active = 0;
    int first = -1;    /* first non-release entry (hit-test anchor) */
    int press = -1;    /* first press entry */

    if(count < 0)
        count = 0;
    if(count > X_TOUCH_MAX)
        count = X_TOUCH_MAX;

    _mt_diag.frames++;
    _mt_diag.last_n = count;
    _mt_diag.last_edges[0] = '\0';
    for(int i = 0; i < count; i++) {
        touch_evt_t* t = &ev->value.touch.touchs[i];
        char edge[8];
        snprintf(edge, sizeof(edge), "%u%c ", (unsigned)t->id,
                t->state == TOUCH_STATE_PRESS ? 'P' :
                t->state == TOUCH_STATE_RELEASE ? 'R' :
                t->state == TOUCH_STATE_CANCEL ? 'C' : 'M');
        if(strlen(_mt_diag.last_edges) + strlen(edge) < sizeof(_mt_diag.last_edges))
            strcat(_mt_diag.last_edges, edge);
    }

    /*the gesture owner so far: a release-only frame drops mt_grab below
      (and the synthetic mouse UP clears mouse_grab) BEFORE the raw
      delivery, and it has no live contact to hit-test - remember the
      owner now so the lift still reaches the client*/
    xwin_t* prev_owner = x->current.mt_grab;
    if(prev_owner == NULL)
        prev_owner = x->current.mouse_grab;

    /*frames carry contact EDGES (press/move/release of what changed), not
      the full active set: a stationary finger is absent. Track the down
      ids and their positions across frames so a staggered lift does not
      end the session while a silent finger is still on the panel.*/
    for(int i = 0; i < count; i++) {
        touch_evt_t* t = &ev->value.touch.touchs[i];
        if(t->id < X_TOUCH_MAX) {
            if(t->state == TOUCH_STATE_RELEASE ||
                    t->state == TOUCH_STATE_CANCEL)
                x->current.mt_active_mask &= ~(1u << t->id);
            else {
                x->current.mt_active_mask |= (1u << t->id);
                x->current.mt_last_x[t->id] = t->x;
                x->current.mt_last_y[t->id] = t->y;
                if(t->state == TOUCH_STATE_PRESS) {
                    x->current.mt_press_x[t->id] = t->x;
                    x->current.mt_press_y[t->id] = t->y;
                }
            }
        }
        if(t->state != TOUCH_STATE_RELEASE &&
                t->state != TOUCH_STATE_CANCEL) {
            if(first < 0)
                first = i;
            if(t->state == TOUCH_STATE_PRESS && press < 0)
                press = i;
        }
    }
    for(int i = 0; i < X_TOUCH_MAX; i++) {
        if(x->current.mt_active_mask & (1u << i))
            active++;
    }

    /*the finger owning the synthetic press lifted: deliver its UP first (a
      tap still promotes to CLICK, with the touch jitter slack). The lift
      and the next finger's press can share one frame - synthesizing nothing
      for either used to lose both keystrokes of such a rollover.*/
    if(x->current.mt_synth_down &&
            !(x->current.mt_active_mask & (1u << x->current.mt_synth_id))) {
        int32_t px = x->current.mt_last_x[x->current.mt_synth_id];
        int32_t py = x->current.mt_last_y[x->current.mt_synth_id];
        bool culled = false;
        for(int i = 0; i < count; i++) {
            touch_evt_t* t = &ev->value.touch.touchs[i];
            if((t->state == TOUCH_STATE_RELEASE ||
                    t->state == TOUCH_STATE_CANCEL) &&
                    t->id == (uint32_t)x->current.mt_synth_id) {
                px = t->x;
                py = t->y;
                culled = (t->state == TOUCH_STATE_CANCEL);
                break;
            }
        }
        x->current.mt_click_slack = MT_CLICK_SLACK;
        /*a grip-filter cull must never tap: the culled contact may have
          rested stationary well inside the promotion distance. mouse_handle
          resets mt_no_click on every return path.*/
        x->current.mt_no_click = culled;
        mt_synth_mouse(x, MOUSE_STATE_UP, px, py);
        x->current.mt_synth_down = false;
        x->current.mt_pending = false;
    }

    /*session transitions and single-finger mouse compatibility first, so
      the raw delivery below can follow the mouse grab of a lone finger*/
    if(active >= 2) {
        if(!x->current.mt_session) {
            if(x->current.mt_synth_down) {
                /*a second finger landed over the lone press: defer the
                  classification, a brief overlap is typing rollover*/
                if(!x->current.mt_pending) {
                    x->current.mt_pending = true;
                    mt_midpoint(x, &x->current.mt_pending_cx,
                            &x->current.mt_pending_cy);
                }
            }
            else {
                x->current.mt_session = true;
            }
        }
        if(x->current.mt_pending) {
            int32_t cx = 0, cy = 0;
            mt_midpoint(x, &cx, &cy);
            if(abs_32(cx - x->current.mt_pending_cx) > MT_MULTI_SLOP ||
                    abs_32(cy - x->current.mt_pending_cy) > MT_MULTI_SLOP) {
                /*real two-finger gesture: cancel the lone press without
                  letting it click. mt_no_click keeps mouse_xwin_handle from
                  promoting the synthetic release to CLICK - otherwise a
                  drag would type the key under the first finger. The plain
                  UP still clears any pressed key/button state in the client.*/
                /*the pending classification consumed the press frame, so the
                  multi-press grab acquisition below (press>=0 with mt_session
                  already true) never fires for a drag grown out of a lone
                  press: adopt that press's window NOW, before the cancel UP
                  drops mouse_grab, or the release-only lift finds no owner
                  and is dropped - the client never sees the drag end*/
                if(x->current.mt_grab == NULL)
                    x->current.mt_grab = x->current.mouse_grab;
                x->current.mt_no_click = true;
                mt_synth_mouse(x, MOUSE_STATE_UP,
                        x->current.mt_last_x[x->current.mt_synth_id],
                        x->current.mt_last_y[x->current.mt_synth_id]);
                x->current.mt_no_click = false;
                x->current.mt_synth_down = false;
                x->current.mt_pending = false;
                x->current.mt_session = true;
            }
        }
    }
    else if(x->current.mt_session) {
        if(active == 0) {
            x->current.mt_session = false;
            x->current.mt_grab = NULL;
        }
    }
    else {
        /*lone finger: a fresh press, the survivor of a rollover, or the
          second finger of a pending pair lifted first*/
        x->current.mt_pending = false;
        if(active == 1) {
            int id = -1;
            for(int i = 0; i < X_TOUCH_MAX; i++) {
                if(x->current.mt_active_mask & (1u << i)) {
                    id = i;
                    break;
                }
            }
            if(!x->current.mt_synth_down) {
                int32_t px = x->current.mt_last_x[id];
                int32_t py = x->current.mt_last_y[id];
                if(first >= 0) {
                    px = ev->value.touch.touchs[first].x;
                    py = ev->value.touch.touchs[first].y;
                }
                mt_synth_mouse(x, MOUSE_STATE_DOWN, px, py);
                x->current.mt_synth_down = true;
                x->current.mt_synth_id = id;
                x->current.mt_synth_ms = kernel_tic_ms(0);
            }
            else if(first >= 0 &&
                    (kernel_tic_ms(0) - x->current.mt_synth_ms) >= MT_DRAG_HOLD_MS) {
                touch_evt_t* t = &ev->value.touch.touchs[first];
                if(t->x != x->mouse_state.last_pos.x ||
                        t->y != x->mouse_state.last_pos.y) {
                    mt_synth_mouse(x, MOUSE_STATE_DRAG, t->x, t->y);
                }
            }
        }
    }

    /*raw frame delivery: mt_grab outranks, then a persistent popup grab,
      then the mouse grab of a lone finger, then a fresh hit-test*/
    xwin_t* win = x->current.mt_grab;
    if(win != NULL && (win->xinfo == NULL || !win->xinfo->visible)) {
        win = NULL;
        x->current.mt_grab = NULL;
    }
    if(win == NULL && x->current.popup_grab != NULL) {
        xwin_t* gwin = x->current.popup_grab;
        if(gwin->xinfo == NULL || !gwin->xinfo->visible)
            x->current.popup_grab = NULL;
        else
            win = gwin;
    }
    if(win == NULL && !x->current.mt_session && x->current.mouse_grab != NULL) {
        if(x->current.mouse_grab->xinfo == NULL ||
                !x->current.mouse_grab->xinfo->visible)
            x->current.mouse_grab = NULL; /*hidden meanwhile: stale grab*/
        else
            win = x->current.mouse_grab;
    }
    if(win == NULL && first >= 0) {
        touch_evt_t* t = &ev->value.touch.touchs[first];
        win = get_owner_at(x, t->x, t->y, NULL);
        if(win != NULL && x->current.mt_session && press >= 0 &&
                x->current.mt_grab == NULL) {
            /*multi-finger press: bring the window up like a mouse DOWN and
              hold the contact grab until every finger lifts*/
            if(win != x->win_tail)
                xwin_top(x, win);
            else
                try_focus(x, win);
            x->current.mt_grab = win;
        }
    }
    if(win == NULL && first < 0 && prev_owner != NULL &&
            prev_owner->xinfo != NULL && prev_owner->xinfo->visible) {
        /*release-only frame: the grabs were just dropped and there is no
          live contact to hit-test - deliver the lift to the window that
          owned the gesture, or its contact state never clears*/
        win = prev_owner;
    }
    if(win != NULL) {
        /*clients only know press/move/release: a grip cull reaches them as
          a plain lift of that contact*/
        for(int i = 0; i < count; i++) {
            if(ev->value.touch.touchs[i].state == TOUCH_STATE_CANCEL)
                ev->value.touch.touchs[i].state = TOUCH_STATE_RELEASE;
        }
        x_push_event(x, win, ev);
        _mt_diag.delivered++;
    }
    else {
        _mt_diag.dropped++;
    }
    if(first < 0)
        _mt_diag.rel_only++;

    x_repaint_req(x, x->current_display);
}

static int im_handle(x_t* x, int32_t from_pid, xevent_t* ev) {
    if(ev->state == XIM_STATE_PRESS && x->win_focus)
        x->im_state.down_win_fd = x->win_focus->fd;

    if(x->im_state.win_xim_actived && x->im_state.win_xim != NULL && from_pid != x->im_state.win_xim->from_pid) {
        x_push_event(x, x->im_state.win_xim, ev);
    }
    else if(x->win_focus != NULL && x->im_state.down_win_fd == x->win_focus->fd) {
        x_push_event(x, x->win_focus, ev);
    }
    return 0;
}

/*touchpad input landing within this window after an IME event is dropped:
  the fingers that just tapped the on-screen keyboard lift onto the touchpad
  and would otherwise click through into the focused window*/
#define X_IM_TPAD_SUPPRESS_MS 300

/*mouse <-> touch switch: the cursor assets (png, size, offsets) are loaded
  per type by cursor_init (cursors/mouse.json vs cursors/touch.json), so
  flipping cursor.type alone keeps drawing the old asset set - a touch
  press would still show the mouse arrow. Erase the on-screen cursor with
  the OLD geometry first (same rule as x_cursor_set_busy), then reload.*/
static void cursor_type_switch(x_t* x, uint32_t type) {
    if(x->cursor.type == type)
        return;

    if(x->cursor.saved != NULL) {
        hide_cursor(x);
        graph_free(x->cursor.saved);
        x->cursor.saved = NULL;
    }

    x->cursor.type = type;
    cursor_init(x->cursor.theme, &x->cursor);
    if(x->mouse_state.busy && x->cursor.img_busy != NULL) {
        x->cursor.size.w = x->cursor.img_busy->w;
        x->cursor.size.h = x->cursor.img_busy->h;
        x->cursor.offset.x = x->cursor.offset_busy.x;
        x->cursor.offset.y = x->cursor.offset_busy.y;
    }

    /*the asset swap may change the cursor rect: a full repaint guarantees
      the erase above reaches the panel even where the new (smaller) cursor
      rect would not cover the old image*/
    x_dirty(x, x->current_display);
}

void handle_input(x_t* x, int32_t from_pid, xevent_t* ev) {
    /*display asleep: drop all input. The panel is black and the
      compositor is paused; wake comes from powerd's power-key polling,
      which runs outside this process, so nothing needs to be queued for
      a burst on resume (a touch storm must not repaint the dark panel
      through the cursor fast path either).*/
    if(x->sleeping)
        return;

    if(ev->type == XEVT_MOUSE || ev->type == XEVT_MOUSE_TPAD)
        cursor_type_switch(x, CURSOR_MOUSE);
    else if(ev->type == XEVT_TOUCH || ev->type == XEVT_TOUCH_MT)
        cursor_type_switch(x, CURSOR_TOUCH);

    if(ev->type == XEVT_IM || ev->type == XEVT_IM_VKEYB) {
        if(ev->type == XEVT_IM) //virtual keyboard events don't arm the suppression
            x->im_state.last_im_ms = kernel_tic_ms(0);
        ev->type = XEVT_IM;
        im_handle(x, from_pid, ev);
    }
    else if(ev->type == XEVT_MOUSE_TPAD) {
        uint64_t now = kernel_tic_ms(0);
        if(x->im_state.last_im_ms != 0 &&
                (now - x->im_state.last_im_ms) < X_IM_TPAD_SUPPRESS_MS)
            return; //too soon after an IME event: ignore the touchpad input
        ev->type = XEVT_MOUSE;
        mouse_handle(x, ev);
        x_repaint_req(x, x->current_display);
    }
    else if(ev->type == XEVT_MOUSE || ev->type == XEVT_TOUCH) {
        ev->type = XEVT_MOUSE;
        mouse_handle(x, ev);
        x_repaint_req(x, x->current_display);
    }
    else if(ev->type == XEVT_TOUCH_MT) {
        touch_mt_handle(x, ev);
    }
}
