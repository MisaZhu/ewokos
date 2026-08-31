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
    if(win->frame_g != NULL) {
        graph_free(win->frame_g);
        win->frame_g = NULL;
    }
    if(win->xinfo != NULL) {
        win->xinfo->ws_g_shm_id = -1;
        win->xinfo->ws_g_shm_contig = false;
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
  the non-cacheable shm window. Only ever called from inside the blocking
  UPDATE IPC: the client is suspended there, so ws_g cannot change under
  the copy.*/
static void x_update_copy(x_t* x, xwin_t* win) {
    graph_blt(win->ws_g, 0, 0, win->ws_g->w, win->ws_g->h,
            win->ws_g_buffer, 0, 0, win->ws_g->w, win->ws_g->h);

    win->ready = true;
    win->not_ready_ms = 0;
    win_dirty(x, win);
}

int x_update(int fd, int from_pid, x_t* x) {
    if(fd < 0)
        return -1;
    
    xwin_t* win = x_get_win(x, fd, from_pid);
    if(win == NULL || win->xinfo == NULL || win->ws_g == NULL)
        return -1;

    if(!win->xinfo->visible)
        return 0;

    if(win->ws_g_buffer == NULL)
        return -1;

    /*a copy of this window is already queued for the next step. The copy
      must not be redone here either: stacked UPDATEs stay O(1) so
      fast-repainting clients cannot clog the IPC queue that mouse input
      and event delivery share with them (the snapshot copy for one window
      is a full-workspace pass on slow hardware). Remember the drop so the
      step can ask the client for a fresh repaint when none follows.*/
    if(win->refresh_pending) {
        win->update_overtaken = true;
        return 0;
    }

    x_update_copy(x, win);
    win->refresh_pending = true;
    /*this copy picked up the freshest content: a recovery repaint for an
      earlier drop is no longer needed*/
    win->update_overtaken = false;
    win->repaint_grace = 0;
    return 0;
}

/*runs once per step (under the server lock, before compositing): releases
  the per-window update slot so the next UPDATE IPC may snapshot again. The
  snapshot copy itself deliberately does NOT happen here: outside the
  blocking UPDATE IPC the client is free to render into ws_g, so a
  step-time detect+copy would read a half-drawn frame and composite it
  (partial flicker, fullscreen inconsistency). A dropped UPDATE whose
  client stops repainting is recovered by pushing XEVT_WIN_REPAINT after a
  short grace period: the client resends its content through the race-free
  UPDATE IPC path.*/
void x_refresh_pending_updates(x_t* x) {
    xwin_t* win = x->win_head;
    while(win != NULL) {
        if(win->refresh_pending)
            win->refresh_pending = false;

        if(win->update_overtaken) {
            if(win->repaint_grace < 2) {
                win->repaint_grace++;
            }
            else {
                /*no fresh UPDATE arrived since the drop: ask the client
                  to repaint so its latest content gets snapshotted*/
                win->repaint_grace = 0;
                win->update_overtaken = false;
                if(win->xinfo != NULL && win->xinfo->visible) {
                    xevent_t ev;
                    memset(&ev, 0, sizeof(xevent_t));
                    ev.type = XEVT_WIN;
                    ev.value.window.event = XEVT_WIN_REPAINT;
                    x_push_event(x, win, &ev);
                }
            }
        }
        else {
            win->repaint_grace = 0;
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
