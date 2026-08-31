/*handlers for the X protocol requests: window geometry updates, frame
  areas, themes, desktop space and the IPC interplay with xwm*/
#include <string.h>
#include <sys/shm.h>
#include <ewoksys/ipc.h>
#include <ewoksys/proc.h>
#include <x/xwm.h>
#include "xwin_cmd.h"
#include "xwin.h"
#include "xinput.h" //get_mouse_owner
#include "xtheme.h"

static int x_update_frame_areas(x_t* x, xwin_t* win) {
    if(!check_xwm(x))
        return -1;

    if((win->xinfo->style & XWIN_STYLE_NO_FRAME) != 0)
        return -1;

    proto_t in, out;
    PF->init(&out);
    PF->init(&in)->add(&in, win->xinfo, sizeof(xinfo_t));
    int res = ipc_call(x->xwm_pid, XWM_CNTL_GET_FRAME_AREAS, &in, &out);
    PF->clear(&in);

    proto_read_to(&out, &win->r_title, sizeof(grect_t));
    proto_read_to(&out, &win->r_close, sizeof(grect_t));
    proto_read_to(&out, &win->r_min, sizeof(grect_t));
    proto_read_to(&out, &win->r_max, sizeof(grect_t));
    proto_read_to(&out, &win->r_resize, sizeof(grect_t));
    PF->clear(&out);
    return res;
}

static void x_get_min_size(x_t* x, xwin_t* win, int *w, int* h) {
    *w = 100;
    *h = 100;

    if(!check_xwm(x))
        return;

    proto_t in, out;
    PF->init(&out);
    PF->init(&in)->add(&in, win->xinfo, sizeof(xinfo_t));
    int res = ipc_call(x->xwm_pid, XWM_CNTL_GET_MIN_SIZE, &in, &out);
    PF->clear(&in);
    if(res == 0) { 
        *w = proto_read_int(&out);
        *h = proto_read_int(&out);
    }
    PF->clear(&out);
}

static int get_xwm_win_space(x_t* x, int style, int state, grect_t* rin, grect_t* rout) {
    memcpy(rout, rin, sizeof(grect_t));
    if(!check_xwm(x))
        return 0;

    proto_t in, out;
    PF->init(&out);
    PF->format(&in, "i,i,m", (ewokos_addr_t)style, (ewokos_addr_t)state, rin, sizeof(grect_t));

    int res = ipc_call(x->xwm_pid, XWM_CNTL_GET_WIN_SPACE, &in, &out);
    PF->clear(&in);
    if(res == 0)
        proto_read_to(&out, rout, sizeof(grect_t));
    else {
        /*xwm died or refused in flight: fall back to the undecorated
          geometry exactly like the no-xwm case instead of failing the
          whole update. A framed window cannot legitimately have
          winr == wsr, and xwin_revalidate_geometry uses exactly that
          tell to repair the window once xwm answers again. Failing
          here would be worse: xwin_open does not retry, so a window
          created in this moment would never get buffers at all*/
        check_xwm(x); //refresh the liveness state
    }
    PF->clear(&out);

    return 0;
}

/*winr comes from xwm and can go stale: the window may have been sized
  while xwm was down (the fallback then makes winr equal wsr), the theme
  may have changed since, or xwm may have been replaced. A framed window
  the theme decorates can never have winr == wsr, so that equality is the
  tell: fetch the geometry again and rebuild the frame buffer at the
  corrected size. The workspace buffer is untouched, the client does not
  notice any of this.*/
void xwin_revalidate_geometry(x_t* x, xwin_t* win) {
    if(win->xinfo == NULL || !win->ready)
        return;
    if((win->xinfo->style & XWIN_STYLE_NO_FRAME) != 0)
        return;

    /*mirrors what xwm's getWinSpace adds around the workspace*/
    bool edge = win->xinfo->state == XWIN_STATE_MAX ||
            win->xinfo->state == XWIN_STATE_FULL_SCREEN;
    bool deco = ((win->xinfo->style & XWIN_STYLE_NO_TITLE) == 0 &&
            win->xinfo->state != XWIN_STATE_FULL_SCREEN &&
            x->config.xwm_theme.titleH > 0) ||
            (!edge && (x->config.xwm_theme.frameW > 0 ||
            x->config.xwm_theme.shadow > 0));
    if(!deco)
        return;
    if(memcmp(&win->xinfo->winr, &win->xinfo->wsr, sizeof(grect_t)) != 0)
        return;
    if(!check_xwm(x))
        return;

    grect_t winr;
    if(get_xwm_win_space(x, (int)win->xinfo->style, (int)win->xinfo->state,
            &win->xinfo->wsr, &winr) != 0)
        return;
    if(memcmp(&winr, &win->xinfo->winr, sizeof(grect_t)) == 0)
        return;

    memcpy(&win->xinfo->winr, &winr, sizeof(grect_t));

    /*publishes -1 right away: a failure below leaves the window without a
      frame, and an id pointing at freed memory is worse than no id*/
    if(win->frame_g != NULL) {
        graph_free(win->frame_g);
        win->frame_g = NULL;
    }
    win->xinfo->frame_g_shm_id = -1;

    /*graph_new_shm allocates its own keyed shm canvas: the buffer and the
      shm id both travel inside frame_g, no window-level mirrors*/
    win->frame_g = graph_new_shm(win->xinfo->winr.w, win->xinfo->winr.h);
    if(win->frame_g == NULL)
        return;
    win->xinfo->frame_g_shm_id = win->frame_g->shm_id;
    win->frame_dirty = true;
    win->shadow_valid = false;
    x_update_frame_areas(x, win);
}

int do_xwin_top(int fd, int from_pid, x_t* x) {
    if(fd < 0)
        return -1;
    
    xwin_t* win = x_get_win(x, fd, from_pid);
    if(win == NULL || win->xinfo == NULL)
        return -1;
    if(!win->xinfo->visible)
        return 0;
    xwin_top(x, win);
    return 0;
}

int do_xwin_try_focus(int fd, int from_pid, x_t* x) {
    if(fd < 0)
        return -1;
    
    xwin_t* win = x_get_win(x, fd, from_pid);
    if(win == NULL || win->xinfo == NULL)
        return -1;
    if(!win->xinfo->visible)
        return 0;

    xwin_top(x, win);
    try_focus(x, win);
    return 0;
}

int do_xwin_set_busy(int fd, int from_pid, proto_t* in, x_t* x) {
    if(fd < 0)
        return -1;

    xwin_t* win = x_get_win(x, fd, from_pid);
    if(win == NULL)
        return -1;

    win->busy = (bool)proto_read_int(in);

    if(get_mouse_owner(x, NULL) == win)
        x_cursor_set_busy(x, win->busy);
    return 0;
}

static void mark_all_frame_dirty(x_t* x, int32_t disp_index) {
    xwin_t* w = x->win_tail; 
    while(w != NULL) {
        xwin_t* p = w->prev;
        if(w->xinfo != NULL &&
                (w->xinfo->display_index == (uint32_t)disp_index || disp_index < 0))
            w->frame_dirty = true; //mark dirty temporary
        w = p;
    }
    x_dirty(x, disp_index);
}

int xwin_update_info(int fd, int from_pid, proto_t* in, proto_t* out, x_t* x) {
    if(fd < 0)
        return -1;

    int32_t xinfo_shm_id = proto_read_int(in);
    if(xinfo_shm_id == -1)
        return -1;
    uint8_t type = proto_read_int(in);
    
    xwin_t* win = x_get_win(x, fd, from_pid);
    if(win == NULL)
        return -1;

    if(win->xinfo == NULL)
        win->xinfo = shmat(xinfo_shm_id, 0, 0);
    if(win->xinfo == (void*)-1) {
        win->xinfo = NULL;
        return -1;
    }
    if(win->xinfo->ws_g_shm_id == 0 && win->ws_g == NULL)
        win->xinfo->ws_g_shm_id = -1;
    if(win->xinfo->frame_g_shm_id == 0 && win->frame_g == NULL)
        win->xinfo->frame_g_shm_id = -1;

    if((win->xinfo->style & XWIN_STYLE_LAUNCHER) != 0)
        x->win_launcher = win;
    if((win->xinfo->style & XWIN_STYLE_XIM) != 0)
        x->im_state.win_xim = win;

    int wsr_w = win->xinfo->wsr.w;
    int wsr_h = win->xinfo->wsr.h;
    int winr_w = win->xinfo->winr.w;
    int winr_h = win->xinfo->winr.h;
    
    if((win->xinfo->style & XWIN_STYLE_NO_FRAME) == 0 &&
      (win->xinfo->style & XWIN_STYLE_NO_TITLE) == 0) {
        int minw = 0, minh = 0;
        x_get_min_size(x, win, &minw, &minh);
        if(win->xinfo->wsr.w < minw)
            win->xinfo->wsr.w = minw;
        if(win->xinfo->wsr.h < minh)
            win->xinfo->wsr.h = minh;

        int32_t maxh = x->displays[win->xinfo->display_index].g->h - x->config.xwm_theme.titleH;
        /*a fullscreen window has no title, so the display height is its limit*/
        if(win->xinfo->state == XWIN_STATE_FULL_SCREEN)
            maxh = x->displays[win->xinfo->display_index].g->h;
        if(win->xinfo->wsr.h > maxh)
            win->xinfo->wsr.h = maxh;
    }

    if(win->xinfo->state == XWIN_STATE_MAX) {
        win->xinfo->wsr.x = 0;
        win->xinfo->wsr.w = x->displays[win->xinfo->display_index].g->w;

        if((win->xinfo->style & XWIN_STYLE_NO_TITLE) == 0) {
            win->xinfo->wsr.y = x->config.xwm_theme.titleH;
            win->xinfo->wsr.h = x->displays[win->xinfo->display_index].g->h - x->config.xwm_theme.titleH;
        }
        else {
            win->xinfo->wsr.y = 0;
            win->xinfo->wsr.h = x->displays[win->xinfo->display_index].g->h;
        }
    }

    if(wsr_w != win->xinfo->wsr.w || wsr_h != win->xinfo->wsr.h) {
        type = type | X_UPDATE_REBUILD | X_UPDATE_REFRESH;
    }

    if(get_xwm_win_space(x, (int)win->xinfo->style, (int)win->xinfo->state,
            &win->xinfo->wsr,
            &win->xinfo->winr) != 0)	
        return -1;

    /* frame_g is sized from winr, not wsr. Theme/style/title changes can
     * change the outer frame size even when the workspace size stays the
     * same, so force a rebuild whenever winr geometry changes. */
    if(winr_w != win->xinfo->winr.w || winr_h != win->xinfo->winr.h) {
        type = type | X_UPDATE_REBUILD | X_UPDATE_REFRESH;
    }
    
    if((type & X_UPDATE_REBUILD) != 0 ||
            win->ws_g == NULL ||
            win->frame_g == NULL) {

        if(win->ws_g != NULL) {
            graph_free(win->ws_g);
            win->ws_g = NULL;
        }
        win->xinfo->ws_g_shm_id = -1;
        win->xinfo->ws_g_shm_contig = false;

        if(win->ws_g_buffer != NULL) {
            graph_free(win->ws_g_buffer);
            win->ws_g_buffer = NULL;
        }

        if(win->frame_g != NULL) {
            graph_free(win->frame_g);
            win->frame_g = NULL;
        }
        win->xinfo->frame_g_shm_id = -1;
        win->xinfo->frame_g_shm_contig = false;

        win->frame_dirty = true;
        win->ready = false;
        win->not_ready_ms = 0; //restart the stuck-window timeout
        /*the old workspace shm is gone: any queued snapshot or recovery
          bookkeeping refers to it*/
        win->refresh_pending = false;
        win->update_overtaken = false;
        win->repaint_grace = 0;

        /*graph_new_shm allocates its own keyed shm canvas: the buffer and
          the shm id both travel inside the graph, no window-level mirrors*/
        win->ws_g = graph_new_shm(win->xinfo->wsr.w, win->xinfo->wsr.h);
        if(win->ws_g == NULL)
            return -1;
        win->xinfo->ws_g_shm_id = win->ws_g->shm_id;
        /*publish the backing type with the id: clients mark their workspace
          graph with it, and g2d only engages on contig-backed canvases*/
        win->xinfo->ws_g_shm_contig = win->ws_g->shm_contig;
        graph_clear(win->ws_g, 0x0);
        win->ws_g_buffer = graph_new_shm(win->xinfo->wsr.w, win->xinfo->wsr.h);
        graph_clear(win->ws_g_buffer, 0x0);

        win->frame_g = graph_new_shm(win->xinfo->winr.w, win->xinfo->winr.h);
        if(win->frame_g == NULL) {
            graph_free(win->ws_g);
            win->ws_g = NULL;
            win->xinfo->ws_g_shm_id = -1;
            win->xinfo->ws_g_shm_contig = false;
            if(win->ws_g_buffer != NULL) {
                graph_free(win->ws_g_buffer);
                win->ws_g_buffer = NULL;
            }
            return -1;
        }
        win->xinfo->frame_g_shm_id = win->frame_g->shm_id;
        win->xinfo->frame_g_shm_contig = win->frame_g->shm_contig;
    }
    x_update_frame_areas(x, win);
    if((type & X_UPDATE_REFRESH) != 0 || win->xinfo->alpha) {
        mark_all_frame_dirty(x, win->xinfo->display_index);
    }
    /*a hidden window's shadow gets painted over by whatever moves in below
      it, so it has to be blended fresh when the window shows again*/
    if(!win->xinfo->visible)
        win->shadow_valid = false;
    return 0;
}

int x_win_space(x_t* x, proto_t* in, proto_t* out) {
    grect_t r;
    int style = proto_read_int(in);
    int state = proto_read_int(in);
    /*a query out of range is treated as a normal window, the same way an
      unknown state would be*/
    if(state < XWIN_STATE_NORMAL || state > XWIN_STATE_FULL_SCREEN)
        state = XWIN_STATE_NORMAL;
    proto_read_to(in, &r, sizeof(grect_t));
    get_xwm_win_space(x, style, state, &r, &r); 
    PF->add(out, &r, sizeof(grect_t));
    return 0;
}

static int x_repaint_all_win(x_t* x) {
    xevent_t ev;
    ev.type = XEVT_WIN;
    ev.value.window.event = XEVT_WIN_REPAINT;

    xwin_t* win = x->win_tail;
    while(win != NULL) {
        if(win->xinfo != NULL) 
            win->xinfo->update_theme = true;
        x_push_event(x, win, &ev);
        win = win->prev;
    }
    return 0;
}

int x_dev_load_theme(x_t* x, proto_t* in, proto_t* out) {
    PF->clear(out);
    const char* name = proto_read_str(in);
    if(name == NULL)
        return -1;
    return x_load_theme(name, &x->config.theme);
}

int x_dev_get_theme(x_t* x, proto_t* in, proto_t* out) {
    PF->clear(out);
    PF->add(out, &x->config.theme, sizeof(x_theme_t));
    return 0;
}

int x_dev_set_theme(x_t* x, proto_t* in, proto_t* out) {
    int32_t sz;
    x_theme_t* theme = (x_theme_t*)proto_read(in, &sz);
    if(theme == NULL || sz != sizeof(x_theme_t))
        return -1;
    memcpy(&x->config.theme, theme, sz);
    x_repaint_all_win(x);
    return 0;
}

static int xwm_theme_update(x_t* x) {
    if(!check_xwm(x))
        return 0;

    proto_t in;
    PF->init(&in)->add(&in, &x->config.xwm_theme, sizeof(xwm_theme_t));
    int res = ipc_call(x->xwm_pid, XWM_CNTL_SET_THEME, &in, NULL);
    PF->clear(&in);

    /*the outer size of every window depends on the theme, so all of them
      are checked against it and resized where needed*/
    if(res == 0) {
        xwin_t* win = x->win_head;
        while(win != NULL) {
            xwin_revalidate_geometry(x, win);
            win = win->next;
        }
    }

    x_dirty(x, -1);
    return res;
}

int x_dev_load_xwm_theme(x_t* x, proto_t* in, proto_t* out) {
    PF->clear(out);
    const char* name = proto_read_str(in);
    if(name == NULL)
        return -1;

    cursor_init(name, &x->cursor);
    if(x_load_xwm_theme(name, &x->config.xwm_theme) != 0)
        return -1;
    return xwm_theme_update(x);
}

int x_dev_get_xwm_theme(x_t* x, proto_t* in, proto_t* out) {
    PF->clear(out);
    PF->add(out, &x->config.xwm_theme, sizeof(xwm_theme_t));
    return 0;
}

int x_dev_set_xwm_theme(x_t* x, proto_t* in, proto_t* out) {
    int32_t sz;
    xwm_theme_t* theme = (xwm_theme_t*)proto_read(in, &sz);
    if(theme == NULL || sz != sizeof(xwm_theme_t))
        return -1;
    memcpy(&x->config.xwm_theme, theme, sz);
    return xwm_theme_update(x);
}

int x_get_desktop_space(x_t* x, proto_t* in, proto_t* out) {
    uint8_t disp_index = proto_read_int(in);

    PF->clear(out);
    if(disp_index >= DISP_MAX) {
        PF->addi(out, -1);
        return -1;
    }

    x_display_t* disp = &x->displays[disp_index];
    PF->addi(out, 0);
    PF->add(out, &disp->desktop_rect, sizeof(grect_t));
    return 0;
}

int x_set_desktop_space(x_t* x, proto_t* in, proto_t* out) {
    uint8_t disp_index = proto_read_int(in);

    PF->clear(out);
    if(disp_index >= DISP_MAX) {
        PF->addi(out, -1);
        return -1;
    }

    x_display_t* disp = &x->displays[disp_index];
    grect_t r;
    proto_read_to(in, &r, sizeof(grect_t));
    PF->addi(out, 0);

    if(r.x == disp->desktop_rect.x &&
            r.y == disp->desktop_rect.y &&
            r.w == disp->desktop_rect.w &&
            r.h == disp->desktop_rect.h)
        return 0;
    memcpy(&disp->desktop_rect, &r, sizeof(grect_t));

    xevent_t ev;
    ev.type = XEVT_WIN;
    ev.value.window.event = XEVT_WIN_REORG;

    xwin_t* win = x->win_head;
    while(win != NULL) {
        if(win->xinfo != NULL && win->xinfo->display_index == disp_index) {
            x_push_event(x, win, &ev);
        }
        win = win->next;
    }
    return 0;
}

int xwin_call_xim(x_t* x, proto_t* in, proto_t* out) {
    if(x->im_state.win_xim == NULL) {
        PF->clear(out)->addi(out, -1);
        return -1;
    }

    if(proto_read_int(in) == 0)
        hide_win(x, x->im_state.win_xim);
    else
        show_win(x, x->im_state.win_xim);
    PF->clear(out)->addi(out, 0);
    return 0;
}
