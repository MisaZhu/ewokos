/*window list management, focus handling, client events and the
  workspace snapshot/dirty bookkeeping of the x server*/
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <ewoksys/vfs.h>
#include <ewoksys/proc.h>
#include <ewoksys/ipc.h>
#include "xwin.h"

static void remove_win(x_t* x, xwin_t* win) {
    xwin_t* prev = win->prev;
    while(prev != NULL) {
        prev = prev->prev;
    }

    if(win->prev != NULL)
        win->prev->next = win->next;
    if(win->next != NULL)
        win->next->prev = win->prev;
    if(x->win_tail == win)
        x->win_tail = win->prev;
    if(x->win_head == win)
        x->win_head = win->next;
    win->next = win->prev = NULL;

    if(win->xinfo != NULL)
        x_dirty(x, win->xinfo->display_index);
    else
        x_dirty(x, -1);
}

void x_quit(int from_pid) {
    xevent_remove(from_pid);
}

void x_get_event(int from_pid, proto_t* out) {
    xevent_t evt;
    if(!xevent_pop(from_pid, &evt)) {
        PF->addi(out, -1);
        return;
    }
    PF->addi(out, 0)->add(out, &evt, sizeof(xevent_t));
}

void x_get_event_node(int from_pid, proto_t* out) {
    PF->addi(out, xevent_get_node(from_pid));
}

void x_push_event(x_t* x, xwin_t* win, xevent_t* e) {
    (void)x;
    if(win == NULL || win->from_pid <= 0 || win->xinfo == NULL)
        return;
    e->win = win->xinfo->win;
    xevent_push(win->from_pid, e);
    uint32_t evt_node = xevent_get_node(win->from_pid);
    if(evt_node != 0)
        vfs_wakeup(evt_node, VFS_EVT_RD);
}

void hide_win(x_t* x, xwin_t* win) {
    x->im_state.win_xim_actived = false;
    if(win == NULL)
        return;

    xevent_t e;
    e.type = XEVT_WIN;
    e.value.window.event = XEVT_WIN_VISIBLE;
    e.value.window.v0 = 0;
    x_push_event(x, win, &e);
}

void show_win(x_t* x, xwin_t* win) {
    if(win == NULL)
        return;

    x->im_state.win_xim_actived = true;
    xevent_t e;
    e.type = XEVT_WIN;
    e.value.window.event = XEVT_WIN_VISIBLE;
    e.value.window.v0 = 1;
    x_push_event(x, win, &e);
}

void x_unfocus(x_t* x) {
    if(x->win_focus == NULL)
        return;

    hide_win(x, x->im_state.win_xim);
    xevent_t e;
    e.type = XEVT_WIN;
    e.value.window.event = XEVT_WIN_UNFOCUS;
    x->win_focus->xinfo->focused = false;
    x->win_focus->frame_dirty = true;
    x_push_event(x, x->win_focus, &e);

    proc_priority(x->win_focus->from_pid, x->config.bg_proc_priority);
    x->win_focus = NULL;
}

void try_focus(x_t* x, xwin_t* win) {
    if(x->win_focus == win || win->xinfo == NULL) 
        return;
    if((win->xinfo->style & XWIN_STYLE_NO_FOCUS) == 0 && 
            (win->xinfo->style & XWIN_STYLE_LAZY) == 0) {
        x_unfocus(x);
        xevent_t e;
        e.type = XEVT_WIN;
        e.value.window.event = XEVT_WIN_FOCUS;
        win->xinfo->focused = true;
        win->frame_dirty = true;
        x_push_event(x, win, &e);
        x->win_focus = win;

        proc_priority(x->win_focus->from_pid, 0);
    }
}

void push_win(x_t* x, xwin_t* win) {
    if(win->xinfo == NULL) { //xinfo not ready, just add to head
        if(x->win_head != NULL) {
            x->win_head->prev = win;
            win->next = x->win_head;
            x->win_head = win;
        }
        else {
            x->win_tail = x->win_head = win;
        }
        return;
    }

    if((win->xinfo->style & XWIN_STYLE_SYSBOTTOM) != 0) { //push head if sysbottom style
        if(x->win_head != NULL) {
            x->win_head->prev = win;
            win->next = x->win_head;
            x->win_head = win;
        }
        else {
            x->win_tail = x->win_head = win;
        }
    }
    else if((win->xinfo->style & XWIN_STYLE_SYSTOP) != 0) { //push tail if systop style
        if(x->win_tail != NULL) {
            x->win_tail->next = win;
            win->prev = x->win_tail;
            x->win_tail = win;
        }
        else {
            x->win_tail = x->win_head = win;
        }
    }
    else { 
        xwin_t* win_top = x->win_tail;
        xwin_t* win_systop = NULL;
        while(win_top != NULL) {
            if(win_top->xinfo == NULL) {
                win_top = win_top->prev;
                continue;
            }
            if((win_top->xinfo->style & XWIN_STYLE_SYSTOP) == 0)
                break;
            win_systop = win_top;
            win_top = win_top->prev;
        }

        if(win_top != NULL) {
            win->next = win_top->next;
            if(win_top->next != NULL)
                win_top->next->prev = win;
            else 
                x->win_tail = win;

            win_top->next = win;
            win->prev = win_top;
        }
        else {
            x->win_head = win;
            if(win_systop != NULL)  {
                if(win_systop->prev != NULL)
                    win_systop->prev->next = win;

                win->prev = win_systop->prev;
                win_systop->prev = win;
                win->next = win_systop;
            }
            else {
                x->win_tail = win;
            }
        }
    }
    if(win->xinfo != NULL && win->xinfo->visible)
        try_focus(x, win);
}

xwin_t* get_top_focus_win(x_t* x, bool skip_launcher) {
    xwin_t* ret = x->win_tail; 
    while(ret != NULL) {
        if(ret->xinfo != NULL && ret->xinfo->visible &&
                (ret->xinfo->style & XWIN_STYLE_NO_FOCUS) == 0 &&
                (!skip_launcher || ret != x->win_launcher))
            return ret;
        ret = ret->prev;
    }
    return NULL;
}

xwin_t* get_next_focus_win(x_t* x, bool skip_launcher) {
    xwin_t* ret = x->win_head;
    while(ret != NULL) {
        if(ret->xinfo != NULL && ret->xinfo->visible &&
                (ret->xinfo->style & XWIN_STYLE_NO_FOCUS) == 0 &&
                (!skip_launcher || ret != x->win_launcher))
            return ret;
        ret = ret->next;
    }
    return NULL;
}

void x_del_win(x_t* x, xwin_t* win) {
    if(win == x->win_focus)
        hide_win(x, x->im_state.win_xim);

    /*a painter thread may be parked in proc_block_by(xinfo->win) waiting
      for the step poll to snapshot it. The window is going away and its
      ws_g is about to be freed, so release the handshake first: clear
      the flag and wake the exact thread, otherwise it blocks forever on
      a token nobody will ever fire again. Must happen before graph_free
      below and before shmdt(win->xinfo) at the end.*/
    x_update_release(x, win);

    remove_win(x, win);
    if(win == x->current.win_drag) {
        x->current.win_drag = NULL;
        x->current.drag_state = 0;
        x->current.pos_delta.x = 0;
        x->current.pos_delta.y = 0;
    }
    if(win == x->im_state.win_xim) {
        x->im_state.win_xim = NULL;
        x->im_state.win_xim_actived = false;
    }
    if(x->im_state.down_win_fd == win->fd)
        x->im_state.down_win_fd = -1;
    if(win == x->win_launcher)
        x->win_launcher = NULL;
    if(win == x->win_last)
        x->win_last = NULL;

    if(win->ws_g != NULL) {
        graph_free(win->ws_g);
        win->ws_g = NULL;
    }
    if(win->ws_g2 != NULL) {
        graph_free(win->ws_g2);
        win->ws_g2 = NULL;
    }
    if(win->frame_g != NULL) {
        graph_free(win->frame_g);
        win->frame_g = NULL;
    }
    if(win->xinfo != NULL) {
        win->xinfo->ws_g_shm_id = -1;
        win->xinfo->ws_g_shm_contig = false;
        win->xinfo->ws_g2_shm_id = -1;
        win->xinfo->ws_g2_shm_contig = false;
        win->xinfo->ws_g_buffer_shm_id = -1;
        win->xinfo->ws_g_buffer_shm_contig = false;
        win->xinfo->frame_g_shm_id = -1;
        win->xinfo->frame_g_shm_contig = false;
    }
    
    if(win->ws_g_buffer != NULL) {
        graph_free(win->ws_g_buffer);
        win->ws_g_buffer = NULL;
    }

    shmdt(win->xinfo);
    if(win == x->win_focus)
        x->win_focus = NULL;
    free(win);
    win = get_top_focus_win(x, false);
    x->win_last = get_top_focus_win(x, true);

    if(win != NULL) {
        try_focus(x, win);
    }
}

xwin_t* x_get_win(x_t* x, int fd, int from_pid) {
    xwin_t* win = x->win_head;
    while(win != NULL) {
        if((win->fd == fd || fd < 0) && 
                win->from_main_pid == proc_getpid(from_pid)) {
            if(proc_check_uuid(win->from_main_pid, win->from_main_pid_uuid) == win->from_main_pid_uuid)
                return win;
            else {
                win->from_pid = -1;
                win->from_main_pid = -1;
                win->from_main_pid_uuid = 0;
            }
        }
        win = win->next;
    }
    return NULL;
}

bool has_win_by_main_pid(x_t* x, int main_pid) {
    xwin_t* win = x->win_head;
    while(win != NULL) {
        if(win->from_main_pid == main_pid)
            return true;
        win = win->next;
    }
    return false;
}

xwin_t* x_get_win_by_name(x_t* x, const char* name) {
    xwin_t* win = x->win_head;
    while(win != NULL) {
        if(win->xinfo != NULL && win->xinfo->is_main &&
                strcmp(win->xinfo->name, name) == 0) {
            return win;
        }
        win = win->next;
    }
    return NULL;
}

/*
static xwin_t* get_first_visible_win(x_t* x) {
    xwin_t* ret = x->win_tail; 
    while(ret != NULL) {
        if(ret->xinfo->visible)
            return ret;
        ret = ret->prev;
    }
    return NULL;
}
*/

static void unmark_dirty(x_t* x, xwin_t* win) {
    (void)x;
    xwin_t* v = win->next;
    while(v != NULL) {
        v->dirty_mark = false;
        v = v->next;
    }
}

static void mark_dirty_confirm(x_t* x, xwin_t* win) {
    xwin_t* v = win->next;
    while(v != NULL) {
        if(v->dirty_mark) {
            v->dirty = true;
            v->dirty_mark = false;

            if(v != win && v->xinfo != NULL) {
                if(need_repaint_desktop(x, v))
                    x_dirty(x, v->xinfo->display_index);
                else if(frame_cuts_ws(x, v))
                    v->frame_dirty = true;
            }
        }
        v = v->next;
    }
}

static void mark_dirty(x_t* x, xwin_t* win) {
    if(win == NULL ||
            !win->dirty ||
            win->xinfo == NULL ||
            !win->xinfo->visible) 
        return;

    xwin_t* win_next = win->next;
    xwin_t* top = win->next;
    while(top != NULL) {
        grect_t r;
        if(top->xinfo != NULL && top->xinfo->visible) {
            memcpy(&r, &top->xinfo->winr, sizeof(grect_t));

            grect_t *check_r;
            if(x->config.xwm_theme.frameAlpha || x->config.xwm_theme.shadow > 0)
                check_r = &win->xinfo->winr;
            else
                check_r = &win->xinfo->wsr;

            grect_insect(check_r, &r);
            if(r.w > 0 && r.h > 0)
                top->dirty_mark = true; //mark top win dirty temporary
            
            if(r.x == check_r->x &&
                    r.y == check_r->y &&
                    r.w == check_r->w &&
                    r.h == check_r->h) {
                if(!top->xinfo->alpha &&
                    !need_repaint_desktop(x, top) &&
                    (top->xinfo->focused ||
                    (top->xinfo->style & XWIN_STYLE_NO_BG_EFFECT) != 0)) {
                    /*fully hidden by an opaque workspace above: stop extending
                      upward here, but keep the dirty marks collected so far so
                      the covering window can repaint its frame ring if needed.*/
                    break;
                }
            }
        }
        top = top->next;
    }

    mark_dirty_confirm(x, win);
    if(win->dirty)
        mark_dirty(x, win_next);
}

void check_wins(x_t* x) {
    xwin_t* w = x->win_tail; 
    while(w != NULL) {
        xwin_t* p = w->prev;
        if(w->from_main_pid < 0 || proc_check_uuid(w->from_main_pid, w->from_main_pid_uuid) != w->from_main_pid_uuid) {
            /*the owner died without closing its fd: mirror the close path
              and drop its event pool + anonymous vfs node too, otherwise
              both leak on every crashed/killed client*/
            int main_pid = w->from_main_pid;
            x_del_win(x, w);
            if(main_pid >= 0 && !has_win_by_main_pid(x, main_pid))
                x_quit(main_pid);
        }
        w = p;
    }
}

void xwin_top(x_t* x, xwin_t* win) {
    if(win == x->win_focus)
        return;
    remove_win(x, win);
    push_win(x, win);
}

bool need_repaint_frame(x_t* x, xwin_t* win) {
    if((win->xinfo->style & XWIN_STYLE_NO_FRAME) != 0 && !win->xinfo->alpha)
        return false;

    if(x->config.xwm_theme.bgEffect && !win->xinfo->focused)
        return true;
    /*edge-to-edge windows (maximized/fullscreen) have no translucent frame
      pixels blending with what is below them, so a desktop repaint does not
      invalidate their frame*/
    if(x->config.xwm_theme.frameAlpha && !win_edge_to_edge(win))
        return true;
    return false;
}

/*only paths that read the current display content back need a desktop redraw.
  themed alpha corners do not: the workspace stays opaque and the frame ring
  can be repainted on top by itself. shadow bands are handled separately by
  the compositor too, so shadow alone should not escalate into a whole-display
  repaint.*/
bool need_repaint_desktop(x_t* x, xwin_t* win) {
    if(win->xinfo->alpha)
        return true;
    if((win->xinfo->style & XWIN_STYLE_NO_FRAME) != 0)
        return false;
    if(x->config.xwm_theme.bgEffect && !win->xinfo->focused)
        return true;
    return false;
}

static void win_dirty(x_t* x, xwin_t* win) {
    win->dirty = true;
    mark_dirty(x, win);
    if(win->dirty) {
        if(need_repaint_desktop(x, win)) {
            x_dirty(x, win->xinfo->display_index);
        }
        else if(frame_cuts_ws(x, win))
            win->frame_dirty = true;
    }
    x_repaint_req(x, win->xinfo->display_index);
}

/*copy the whole workspace across to the snapshot the compositor reads
  from. The copy rides graph_blt: contig shm canvases travel through
  /dev/g2d (zero-copy on physical addresses) instead of a cpu pass over
  the non-cacheable shm window. src is the buffer the client finished
  rendering: ws_g in the blocking handshake (client parked, ws_g stable),
  or ws_g[front_index] in fps_async mode (client already moved on to the
  other buffer, so src is stable there too).*/
static void x_update_copy(x_t* x, xwin_t* win, graph_t* src) {
    graph_blt(src, 0, 0, src->w, src->h,
            win->ws_g_buffer, 0, 0, src->w, src->h);

    win->ready = true;
    win->not_ready_ms = 0;
    win_dirty(x, win);
}

/*release a client that is parked on the shm UPDATE handshake without
  snapshotting: the window is going away, is being rebuilt, or turned
  invisible. Clears the request flag and wakes the exact thread that
  published itself into update_pid, so xwin_repaint returns instead of
  blocking forever on a token nobody will ever fire again.*/
void x_update_release(x_t* x, xwin_t* win) {
    (void)x;
    if(win == NULL || win->xinfo == NULL)
        return;

    int32_t pid = win->xinfo->update_pid;
    win->xinfo->update_requested = 0;
    win->xinfo->update_pid = -1;
    __sync_synchronize();
    if(pid >= 0)
        proc_wakeup_by(pid, win->xinfo->win);
}

/*runs once per step (under the server lock, before compositing): scans
  every window's shm handshake flag and snapshots the ones that asked for
  it. This replaces the old XWIN_CNTL_UPDATE IPC path entirely - no vdevice
  dispatch, no file cache lookup, no fsinfo_t round-trip on the hot path.
  Two modes:
  - fps_async=0 (blocking): the client is parked in proc_block_by(xinfo->win)
    while we copy ws_g, so ws_g is stable; we clear the flag before the copy
    and wake the painter afterwards. Clients that repaint faster than the
    server fps simply stay blocked until the next step picks them up, which
    is exactly the throttling we want.
  - fps_async=1 (double-buffered): the client published a complete frame into
    ws_g[front_index] and already returned to render the other buffer, so we
    snapshot front_index and never wake anyone. We clear the flag AFTER the
    copy: the client flips buffers only when it sees update_requested==0, so
    clearing after the snapshot guarantees it never starts rendering the
    buffer we are reading. No tearing, and the client keeps its own fps.*/
void x_poll_updates(x_t* x) {
    xwin_t* win = x->win_head;
    while(win != NULL) {
        if(win->xinfo != NULL && win->xinfo->update_requested) {
            if(win->xinfo->fps_async) {
                uint32_t fi = win->xinfo->front_index;
                graph_t* src = (fi == 1) ? win->ws_g2 : win->ws_g;
                if(win->xinfo->visible && src != NULL && win->ws_g_buffer != NULL)
                    x_update_copy(x, win, src);
                /*clear AFTER the copy so the client cannot flip onto src
                  until we are done reading it (see header comment)*/
                __sync_synchronize();
                win->xinfo->update_requested = 0;
            } else {
                int32_t pid = win->xinfo->update_pid;
                ewokos_addr_t token = win->xinfo->win;

                /*clear the flag BEFORE the copy so a second painter thread
                  that races in during graph_blt re-asserts it and gets
                  picked up next step instead of being lost*/
                win->xinfo->update_requested = 0;
                win->xinfo->update_pid = -1;
                __sync_synchronize();

                if(win->xinfo->visible && win->ws_g != NULL && win->ws_g_buffer != NULL)
                    x_update_copy(x, win, win->ws_g);

                if(pid >= 0)
                    proc_wakeup_by(pid, token);
            }
        }
        win = win->next;
    }
}

/*
static int xwin_set_visible(int fd, int from_pid, proto_t* in, x_t* x) {
    if(fd < 0)
        return -1;
    
    xwin_t* win = x_get_win(x, fd, from_pid);
    if(win == NULL)
        return -1;

    win->xinfo->visible = proto_read_int(in);
    win->dirty = true;
    x_dirty(x, win->xinfo->display_index);
    return 0;
}
*/

/* whether rect r is fully covered by the opaque workspace of one window
   above 'from' (from==NULL means checking against all windows). */
bool covered_by_opaque_win(x_t* x, xwin_t* from, uint32_t display_index, const grect_t* r) {
    xwin_t* top = (from == NULL) ? x->win_head : from->next;
    while(top != NULL) {
        if(top->ready && top->xinfo != NULL && top->xinfo->visible &&
                top->xinfo->display_index == display_index) {
            if(!top->xinfo->alpha &&
                    !need_repaint_desktop(x, top) &&
                    (top->xinfo->focused ||
                    (top->xinfo->style & XWIN_STYLE_NO_BG_EFFECT) != 0)) {
                /*an edge-to-edge window is opaque across its whole winr:
                  the title strip is solid decoration drawn by xwm, so it
                  covers just like the workspace does*/
                const grect_t* cover = win_edge_to_edge(top) ?
                        &top->xinfo->winr : &top->xinfo->wsr;
                if(rect_contains(cover, r))
                    return true;
            }
        }
        top = top->next;
    }
    return false;
}
