/* multitouch: fullscreen visualizer for XEVT_TOUCH_MT contact frames.
   Every active contact is drawn as a filled circle at its screen position,
   colored by contact id, so the goodixd -> xtouch -> xserverd multi-touch
   pipeline can be verified by eye.  The top-right corner button exits
   (single-finger taps reach it through the server's mouse synthesis). */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <x/x.h>
#include <x/xwin.h>
#include <graph/graph.h>

#define MT_RADIUS 36
#define EXIT_SZ 64

typedef struct {
    uint32_t id;
    int32_t x, y;
    bool active;
} contact_t;

static contact_t _contacts[X_TOUCH_MAX];
static xwin_t* _win = NULL;

/* one distinct color per contact id */
static const uint32_t _palette[X_TOUCH_MAX] = {
    0xffff4444, 0xff44cc44, 0xff4488ff, 0xffffcc00, 0xffff44ff,
    0xff00cccc, 0xffff8800, 0xff88ff00, 0xff8844ff, 0xffffffff
};

static void contacts_clear(void) {
    memset(_contacts, 0, sizeof(_contacts));
}

/* a frame lists every active contact; ids absent from it have lifted */
static void mt_frame(const xevent_t* ev) {
    int n = ev->state;
    if(n < 0)
        n = 0;
    if(n > X_TOUCH_MAX)
        n = X_TOUCH_MAX;

    contacts_clear();
    for(int i = 0; i < n; i++) {
        const touch_evt_t* t = &ev->value.touch.touchs[i];
        if(t->state == TOUCH_STATE_RELEASE)
            continue;
        if(t->id >= X_TOUCH_MAX)
            continue;
        gpos_t p = xwin_get_inside_pos(_win, t->x, t->y);
        _contacts[t->id].id = t->id;
        _contacts[t->id].x = p.x;
        _contacts[t->id].y = p.y;
        _contacts[t->id].active = true;
    }
    xwin_repaint_req(_win);
}

/* legacy single-pointer records (non-MT drivers) show up as contact 0;
   they carry MOUSE_STATE_* states with the position in value.mouse */
static void legacy_touch(const xevent_t* ev) {
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
    else {
        return;
    }
    xwin_repaint_req(_win);
}

static bool in_exit_btn(int32_t x, int32_t y) {
    if(_win == NULL || _win->xinfo == NULL)
        return false;
    int32_t w = _win->xinfo->wsr.w;
    return x >= w - EXIT_SZ && y <= EXIT_SZ;
}

static void on_event(xwin_t* xwin, xevent_t* ev) {
    (void)xwin;
    if(ev->type == XEVT_TOUCH_MT) {
        mt_frame(ev);
    }
    else if(ev->type == XEVT_TOUCH) {
        legacy_touch(ev);
    }
    else if(ev->type == XEVT_MOUSE &&
            ev->state == MOUSE_STATE_CLICK &&
            in_exit_btn(ev->value.mouse.x, ev->value.mouse.y)) {
        x_terminate(_win->x);
    }
}

static void on_repaint(xwin_t* xwin, graph_t* g) {
    (void)xwin;
    graph_clear(g, 0xff101418);

    /* exit button: top-right square with a cross */
    int32_t bx = g->w - EXIT_SZ;
    graph_fill_rect(g, bx, 0, EXIT_SZ, EXIT_SZ, 0xff2a3038);
    graph_line(g, bx + 18, 18, bx + EXIT_SZ - 18, EXIT_SZ - 18, 0xffcccccc);
    graph_line(g, bx + EXIT_SZ - 18, 18, bx + 18, EXIT_SZ - 18, 0xffcccccc);

    int r = x_hidpi_size(MT_RADIUS);
    for(int i = 0; i < X_TOUCH_MAX; i++) {
        const contact_t* c = &_contacts[i];
        if(!c->active)
            continue;
        uint32_t color = _palette[c->id % X_TOUCH_MAX];
        graph_fill_circle(g, c->x, c->y, r, color);
        /* white core marks the exact reported point */
        graph_fill_circle(g, c->x, c->y, r/6, 0xffffffff);
    }
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