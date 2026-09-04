/*window list management, focus handling, client events and the
  workspace accept/dirty bookkeeping of the x server*/
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <ewoksys/vfs.h>
#include <ewoksys/proc.h>
#include <ewoksys/ipc.h>
#include <ewoksys/kernel_tic.h>
#include "xwin.h"

/*how long a published frame may sit unconsumed before its client is let go
  anyway. Normally one step: x_poll_updates accepts, x_repaint composites
  from the very same buffer and commits. This only fires when the frame never
  reached the compositor at all - the display stuck busy, the window covered,
  its display inactive - and in every one of those the content is not on
  screen anyway, so dropping the claim costs a frame, not a picture. Without
  it a blocking client stays parked in proc_block_by and an fps_async client
  stays locked out of its handoff buffer for as long as the condition lasts.*/
#define X_ACCEPT_TIMEOUT_MS 200

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

/*accept a published frame IN PLACE: the buffer the client rendered into is
  the compositor's source from here on, so there is no snapshot to take and
  the whole-frame copy the old x_update_copy did is simply gone. That is the
  whole point of this path - one full-frame move less per window per frame.

  It is safe because the client cannot be writing that buffer while we own
  it: a blocking painter is parked in proc_block_by until x_update_commit
  wakes it, and an fps_async client only starts its handoff copy once
  update_requested is back to 0, which the same commit does after the blit.
  What is left to do here is the bookkeeping - the window became ready, its
  content changed, and xwm has to be pointed at the canvas the compositor
  will actually read.*/
static void x_accept_update(x_t* x, xwin_t* win) {
    graph_t* src = win_comp_src(win);
    if(src != NULL) {
        /*xwm blends decorations over the same pixels the compositor reads, so
          republish the id whenever the source moves (fps_async flips it onto
          the handoff buffer on the first present after a rebuild)*/
        win->xinfo->ws_g_buffer_shm_id = src->shm_id;
        win->xinfo->ws_g_buffer_shm_contig = src->shm_contig;
    }

    win->ready = true;
    win->not_ready_ms = 0;
    win->repaint_req_ms = 0;
    if(win->accept_ms == 0) {
        uint64_t now = kernel_tic_ms(0);
        win->accept_ms = (now == 0) ? 1 : now;
    }
    win_dirty(x, win);
}

/*hand the published buffer back to its client. Called once the compositor is
  done reading it (x_repaint, right after draw_win), or when the frame is not
  going on screen at all (covered window, accept timeout). Deliberately NOT
  called from x_poll_updates any more: releasing there would let the painter
  start the next frame into the very buffer this step is about to composite.

  - fps_async=0 (blocking): the painter is still parked, so clear the flag and
    wake the exact thread that published itself into update_pid.
  - fps_async=1: nobody is waiting, but the flag still has to be cleared AFTER
    the blit - the client copies onto the handoff buffer only when it sees
    update_requested==0, so clearing late is what keeps it off the buffer we
    were reading.*/
void x_update_commit(x_t* x, xwin_t* win) {
    (void)x;
    if(win == NULL || win->xinfo == NULL)
        return;

    win->accept_ms = 0;
    if(!win->xinfo->update_requested)
        return;

    if(win->xinfo->fps_async) {
        __sync_synchronize();
        win->xinfo->update_requested = 0;
        return;
    }

    int32_t pid = win->xinfo->update_pid;
    win->xinfo->update_requested = 0;
    win->xinfo->update_pid = -1;
    __sync_synchronize();
    if(pid >= 0)
        proc_wakeup_by(pid, win->xinfo->win);
}

/*release a client that is parked on the shm UPDATE handshake without
  compositing it: the window is going away, is being rebuilt, or turned
  invisible. Clears the request flag and wakes the exact thread that
  published itself into update_pid, so xwin_repaint returns instead of
  blocking forever on a token nobody will ever fire again.*/
void x_update_release(x_t* x, xwin_t* win) {
    x_update_commit(x, win);
}

/*Abandon an accepted frame that never reached the compositor (its display
  stayed busy or is inactive - see X_ACCEPT_TIMEOUT_MS). Handing the buffer
  back is only half of it: from that moment the client may flip into ws_g2
  again at any time, so the compositor must stop reading it too. A full
  rebuild bypasses win_src_stable, so leaving win->dirty set here would let
  draw_win (and the DRAW_FRAME it hands xwm) sample that buffer while the
  client's next handoff copy is half done - a torn frame and garbled
  decorations. Drop the damage instead; the area keeps whatever the scan-out
  already holds, which is what it held before too since this frame never got
  there, and ask the client for the picture again so an idle one does not
  leave the window blank until its next spontaneous repaint.*/
static void x_accept_abandon(x_t* x, xwin_t* win) {
    win->dirty = false;
    win->frame_dirty = false;
    x_update_commit(x, win);

    if(win->xinfo != NULL && win->xinfo->visible) {
        xevent_t ev;
        memset(&ev, 0, sizeof(xevent_t));
        ev.type = XEVT_WIN;
        ev.value.window.event = XEVT_WIN_REPAINT;
        x_push_event(x, win, &ev);
    }
}

/*runs once per step (under the server lock, before compositing): scans every
  window's shm handshake flag and accepts the frames that were published. This
  replaces the old XWIN_CNTL_UPDATE IPC path entirely - no vdevice dispatch, no
  file cache lookup, no fsinfo_t round-trip on the hot path - and it does not
  copy anything: the accepted buffer is composited as-is later in this step and
  handed back by x_update_commit. Clients that repaint faster than the server
  fps stay blocked (or stay locked out of their handoff buffer) until a step
  picks them up, which is exactly the throttling we want.*/
void x_poll_updates(x_t* x) {
    uint64_t now = kernel_tic_ms(0);
    xwin_t* win = x->win_head;
    while(win != NULL) {
        if(win->xinfo != NULL) {
            if(win->xinfo->update_requested) {
                /*the client published with a release barrier only; pair it here.
                  Without this acquire the load of front_index (which selects
                  ws_g2) and of the ws_g2 pixels themselves can be reordered
                  ahead of the flag on this core: the compositor then blits the
                  buffer graph_clear left empty, commits it as presented, and
                  the window stays blank forever - the client believes its frame
                  landed and an event-driven app never paints again.*/
                __sync_synchronize();
                if(win->xinfo->visible && win_comp_src(win) != NULL)
                    x_accept_update(x, win);
                else
                    x_update_commit(x, win); //nothing to show: don't hold it parked
            }

            /*an accepted frame that never reached the compositor still holds
              its client off the buffer; bound that (see x_accept_abandon)*/
            if(win->accept_ms != 0 && (now - win->accept_ms) >= X_ACCEPT_TIMEOUT_MS)
                x_accept_abandon(x, win);

            /*a visible window that never became ready is simply not on screen:
              the composite loop skips !ready windows. Its client may well think
              it already presented - an fps_async present skipped while the
              server still owned the handoff buffer is not re-issued by an
              event-driven app whose widget layer already consumed its dirty
              state, and ws_g2 (which win_comp_src picks) is still empty. Ask
              for the frame again; ws_g holds the complete picture, so even a
              client that redraws nothing on the event still flips it over.*/
            if(win->xinfo->visible && !win->ready) {
                if(win->not_ready_ms == 0)
                    win->not_ready_ms = (now == 0) ? 1 : now;
                else if((now - win->not_ready_ms) >= X_NOT_READY_TIMEOUT_MS &&
                        (win->repaint_req_ms == 0 ||
                        (now - win->repaint_req_ms) >= X_NOT_READY_TIMEOUT_MS)) {
                    win->repaint_req_ms = (now == 0) ? 1 : now;
                    xevent_t ev;
                    memset(&ev, 0, sizeof(xevent_t));
                    ev.type = XEVT_WIN;
                    ev.value.window.event = XEVT_WIN_REPAINT;
                    x_push_event(x, win, &ev);
                }
            }
            else if(win->not_ready_ms != 0) {
                win->not_ready_ms = 0;
                win->repaint_req_ms = 0;
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
