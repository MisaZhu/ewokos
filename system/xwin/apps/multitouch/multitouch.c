/* multitouch: fullscreen visualizer for XEVT_TOUCH_MT contact frames.
   Every active contact is drawn as a filled circle at its screen position,
   colored by contact id, so the goodixd -> xtouch -> xserverd multi-touch
   pipeline can be verified by eye.  The top-right corner button exits
   (single-finger taps reach it through the server's mouse synthesis).

   Latency: touch frames arrive faster than the server composites, and every
   blocking repaint replays one queued frame.  Repainting each frame in order
   therefore builds a backlog of stale positions that trails the finger by
   many frame periods.  Instead we coalesce: on a touch event, drain every
   event already queued, keep only the newest contact state, and repaint once.
   The canvas is also updated incrementally (erase the old circles, draw the
   new ones) so a frame costs a few small fills instead of a fullscreen clear. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <x/x.h>
#include <x/xwin.h>
#include <graph/graph.h>

#define MT_RADIUS 36
#define EXIT_SZ 64
#define BG_COLOR 0xff101418
#define MT_DRAIN_MAX 64

typedef struct {
    uint32_t id;
    int32_t x, y;
    bool active;
} contact_t;

static contact_t _contacts[X_TOUCH_MAX];
/* what is currently on the canvas, so the next frame can erase just that */
static contact_t _drawn[X_TOUCH_MAX];
static int32_t _drawn_r = 0;
static const void* _drawn_buf = NULL;
static int32_t _drawn_w = 0, _drawn_h = 0;
static xwin_t* _win = NULL;

/* one distinct color per contact id */
static const uint32_t _palette[X_TOUCH_MAX] = {
    0xffff4444, 0xff44cc44, 0xff4488ff, 0xffffcc00, 0xffff44ff,
    0xff00cccc, 0xffff8800, 0xff88ff00, 0xff8844ff, 0xffffffff
};

static void contacts_clear(void) {
    memset(_contacts, 0, sizeof(_contacts));
}

/* a frame carries contact EDGES (press/move/release of what changed), not
   the full active set: a stationary finger is absent and keeps its circle;
   a RELEASE edge clears exactly that id */
static void mt_apply(const xevent_t* ev) {
    int n = ev->state;
    if(n < 0)
        n = 0;
    if(n > X_TOUCH_MAX)
        n = X_TOUCH_MAX;

    for(int i = 0; i < n; i++) {
        const touch_evt_t* t = &ev->value.touch.touchs[i];
        if(t->id >= X_TOUCH_MAX)
            continue;
        if(t->state == TOUCH_STATE_RELEASE) {
            _contacts[t->id].active = false;
            continue;
        }
        gpos_t p = xwin_get_inside_pos(_win, t->x, t->y);
        _contacts[t->id].id = t->id;
        _contacts[t->id].x = p.x;
        _contacts[t->id].y = p.y;
        _contacts[t->id].active = true;
    }
}

/* legacy single-pointer records (non-MT drivers) show up as contact 0;
   they carry MOUSE_STATE_* states with the position in value.mouse */
static void legacy_apply(const xevent_t* ev) {
    if(ev->state == MOUSE_STATE_UP) {
        contacts_clear();
    }
    else if(ev->state == MOUSE_STATE_DOWN || ev->state == MOUSE_STATE_DRAG) {
        gpos_t p = xwin_get_inside_pos(_win, ev->value.mouse.x, ev->value.mouse.y);
        contacts_clear();
        _contacts[0].x = p.x;
        _contacts[0].y = p.y;
        _contacts[0].active = true;
    }
}

static bool in_exit_btn(int32_t x, int32_t y) {
    if(_win == NULL || _win->xinfo == NULL)
        return false;
    int32_t w = _win->xinfo->wsr.w;
    return x >= w - EXIT_SZ && y <= EXIT_SZ;
}

static void on_event(xwin_t* xwin, xevent_t* ev) {
    if(ev->type == XEVT_TOUCH_MT || ev->type == XEVT_TOUCH) {
        if(ev->type == XEVT_TOUCH_MT)
            mt_apply(ev);
        else
            legacy_apply(ev);

        /* Coalesce the backlog: apply every touch frame already queued so
           the single repaint below shows the newest positions.  Repaint
           requests queued along the way are swallowed - one repaint covers
           them all; anything else is routed as x_run() would. */
        xevent_t more;
        int guard = 0;
        while(guard++ < MT_DRAIN_MAX && x_poll_event(xwin->x, &more) == 0) {
            if(more.type == XEVT_TOUCH_MT)
                mt_apply(&more);
            else if(more.type == XEVT_TOUCH)
                legacy_apply(&more);
            else if(more.type == XEVT_WIN &&
                    more.value.window.event == XEVT_WIN_REPAINT)
                continue;
            else
                xwin_dispatch_event(xwin->x, &more);
        }
        xwin_repaint_req(xwin);
    }
    else if(ev->type == XEVT_MOUSE &&
            ev->state == MOUSE_STATE_CLICK &&
            in_exit_btn(ev->value.mouse.x, ev->value.mouse.y)) {
        x_terminate(_win->x);
    }
}

static void draw_exit_btn(graph_t* g) {
    int32_t bx = g->w - EXIT_SZ;
    graph_fill_rect(g, bx, 0, EXIT_SZ, EXIT_SZ, 0xff2a3038);
    graph_line(g, bx + 18, 18, bx + EXIT_SZ - 18, EXIT_SZ - 18, 0xffcccccc);
    graph_line(g, bx + EXIT_SZ - 18, 18, bx + 18, EXIT_SZ - 18, 0xffcccccc);
}

static void on_repaint(xwin_t* xwin, graph_t* g) {
    (void)xwin;
    int r = x_hidpi_size(MT_RADIUS);

    /* A fresh or rebuilt canvas needs the full background; otherwise erase
       only what the previous frame drew.  The erase radius covers the old
       circle plus its anti-aliased fringe even if hidpi changed since. */
    if(g->buffer != _drawn_buf || g->w != _drawn_w || g->h != _drawn_h) {
        graph_clear(g, BG_COLOR);
        memset(_drawn, 0, sizeof(_drawn));
    }
    else {
        int er = (_drawn_r > r ? _drawn_r : r) + 1;
        for(int i = 0; i < X_TOUCH_MAX; i++) {
            if(_drawn[i].active)
                graph_fill_circle(g, _drawn[i].x, _drawn[i].y, er, BG_COLOR);
        }
    }

    /* exit button: top-right square with a cross (redrawn in case an erase
       clipped it) */
    draw_exit_btn(g);

    for(int i = 0; i < X_TOUCH_MAX; i++) {
        const contact_t* c = &_contacts[i];
        if(!c->active)
            continue;
        uint32_t color = _palette[c->id % X_TOUCH_MAX];
        graph_fill_circle(g, c->x, c->y, r, color);
        /* white core marks the exact reported point */
        graph_fill_circle(g, c->x, c->y, r/6, 0xffffffff);
    }

    memcpy(_drawn, _contacts, sizeof(_drawn));
    _drawn_r = r;
    _drawn_buf = g->buffer;
    _drawn_w = g->w;
    _drawn_h = g->h;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    x_t x;
    x_init(&x, NULL);

    _win = xwin_open(&x, -1, 0, 0, 320, 240, "multitouch", XWIN_STYLE_NORMAL);
    if(_win == NULL) {
        printf("multitouch: cannot open window\n");
        return 1;
    }
    xwin_set_alpha(_win, false);
    _win->on_event = on_event;
    _win->on_repaint = on_repaint;
    xwin_fullscreen(_win);
    xwin_hide_cursor(_win, true);
    xwin_set_visible(_win, true);

    x_run(&x, _win);
    xwin_destroy(_win);
    return 0;
}