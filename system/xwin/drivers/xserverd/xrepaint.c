/*the per-display repaint pipeline: dirty rect collection, compositing
  order, cursor overlay and the flush to the fb daemon*/
#include <string.h>
#include <ewoksys/kernel_tic.h>
#include "xrepaint.h"
#include "xrender.h"
#include "xwin.h"

#define X_REPAINT_DIRTY_MAX 16

static void x_repaint_add_dirty(graph_t* g, grect_t* rects, uint32_t* num, const grect_t* r) {
    grect_t dirty = *r;
    if(!rect_is_valid(&dirty) || !rect_clip_to_graph(g, &dirty))
        return;

    for(uint32_t i = 0; i < *num; i++) {
        if(rect_contains(&rects[i], &dirty))
            return;
        if(rect_overlap_or_touch(&rects[i], &dirty)) {
            rect_union_to(&rects[i], &dirty);
            uint32_t j = 0;
            while(j < *num) {
                if(i == j) {
                    j++;
                    continue;
                }
                if(rect_overlap_or_touch(&rects[i], &rects[j])) {
                    rect_union_to(&rects[i], &rects[j]);
                    (*num)--;
                    rects[j] = rects[*num];
                    /*the growing union sat in the last slot: the swap-with-
                      last removal just moved it into slot j — follow it, or
                      i keeps growing a dead slot and the live copy gets
                      merged away against its own ghost, losing the whole
                      accumulated region*/
                    if(i == *num)
                        i = j;
                    j = 0;
                    continue;
                }
                j++;
            }
            return;
        }
    }

    if(*num < X_REPAINT_DIRTY_MAX) {
        rects[*num] = dirty;
        (*num)++;
        return;
    }

    rect_union_to(&rects[0], &dirty);
}

/*the fb control block only carries DISPLAY_DIRTY_MAX rects, so merge the
  cheapest pairs together until they fit; without this the daemon has to
  push the whole framebuffer to the panel*/
static uint32_t pack_dirty_rects(const grect_t* rects, uint32_t num,
        grect_t* out, uint32_t max) {
    if(num <= max) {
        memcpy(out, rects, num * sizeof(grect_t));
        return num;
    }

    memcpy(out, rects, max * sizeof(grect_t));
    for(uint32_t i = max; i < num; i++) {
        uint32_t best = 0;
        int64_t best_cost = -1;
        for(uint32_t j = 0; j < max; j++) {
            grect_t u = out[j];
            rect_union_to(&u, &rects[i]);
            int64_t cost = (int64_t)u.w * u.h - (int64_t)out[j].w * out[j].h;
            if(best_cost < 0 || cost < best_cost) {
                best_cost = cost;
                best = j;
            }
        }
        rect_union_to(&out[best], &rects[i]);
    }
    return max;
}

static inline void x_get_cursor_rect(x_t* x, grect_t* r, bool old_pos) {
    int32_t cx = old_pos ? x->cursor.old_pos.x : x->cursor.cpos.x;
    int32_t cy = old_pos ? x->cursor.old_pos.y : x->cursor.cpos.y;
    r->x = cx - x->cursor.offset.x;
    r->y = cy - x->cursor.offset.y;
    r->w = x->cursor.size.w;
    r->h = x->cursor.size.h;
}

void hide_cursor(x_t* x) {
    x_display_t* display = &x->displays[x->current_display];
    if(display->g == NULL)
        return;

    if(x->cursor.saved == NULL) {
        x->cursor.saved = graph_new(NULL, x->cursor.size.w, x->cursor.size.h);
        graph_blt(display->g,
                x->cursor.old_pos.x - x->cursor.offset.x,
                x->cursor.old_pos.y - x->cursor.offset.y,
                x->cursor.size.w,
                x->cursor.size.h,
                x->cursor.saved,
                0,
                0, 
                x->cursor.size.w,
                x->cursor.size.h);
    }
    else  {
        graph_blt(x->cursor.saved,
                0,
                0,
                x->cursor.size.w,
                x->cursor.size.h,
                display->g,
                x->cursor.old_pos.x - x->cursor.offset.x,
                x->cursor.old_pos.y - x->cursor.offset.y,
                x->cursor.size.w,
                x->cursor.size.h);
    }
}

static inline void refresh_cursor(x_t* x) {
    x_display_t* display = &x->displays[x->current_display];
    if(display->g == NULL || x->cursor.saved == NULL)
        return;
    int32_t mx = x->cursor.cpos.x - x->cursor.offset.x;
    int32_t my = x->cursor.cpos.y - x->cursor.offset.y;
    int32_t mw = x->cursor.saved->w;
    int32_t mh = x->cursor.saved->h;

    graph_blt(display->g, mx, my, mw, mh,
            x->cursor.saved, 0, 0, mw, mh);

    draw_cursor(display->g, &x->cursor, mx, my, x->mouse_state.busy);

    x->cursor.old_pos.x = x->cursor.cpos.x;
    x->cursor.old_pos.y = x->cursor.cpos.y;
}

/* X_NOT_READY_TIMEOUT_MS lives in xserver.h: all_win_ready below and the
   stuck-window repaint request in x_poll_updates share the same budget */

static bool all_win_ready(x_t* x) {
    uint64_t now = kernel_tic_ms(0);
    xwin_t* win = x->win_head;
    while(win != NULL) {
        if(win->xinfo != NULL && win->xinfo->visible && !win->ready) {
            if(win->not_ready_ms == 0) {
                win->not_ready_ms = (now == 0) ? 1 : now;
                return false;
            }
            if((now - win->not_ready_ms) < X_NOT_READY_TIMEOUT_MS)
                return false;
            /* stuck too long: stop holding every other window hostage */
        }
        else if(win->not_ready_ms != 0) {
            win->not_ready_ms = 0;
        }
        win = win->next;
    }
    return true;
}

/*the compositor reads each client's own buffer now, so a window whose painter
  is mid-frame must not be composited: its pixels are half-drawn. A published
  frame is always readable - the server owns that buffer until x_update_commit
  hands it back. Otherwise libx's painting flag says whether the client may be
  writing it, and that flag is bracketed around both client styles (on_repaint
  apps draw inside xwin_repaint, framebuffer-style ones draw between presents).
  A client that goes idle after its last frame leaves the flag set for good, so
  the wait is bounded instead of stalling every repaint behind it.*/
#define X_PAINT_TIMEOUT_MS 100

static bool win_src_stable(x_t* x, xwin_t* win) {
    (void)x;
    if(win->xinfo == NULL)
        return false;

    if(win->xinfo->update_requested || !win->xinfo->painting) {
        win->paint_ms = 0;
        return true;
    }

    uint64_t now = kernel_tic_ms(0);
    if(win->paint_ms == 0) {
        win->paint_ms = (now == 0) ? 1 : now;
        return false;
    }
    /*stuck painting: either a genuinely slow renderer or an idle client that
      never published again. It is not going to clear the flag by itself, and
      in the idle case the buffer already holds a complete frame, so stop
      waiting - the same trade X_NOT_READY_TIMEOUT_MS makes above.*/
    return (now - win->paint_ms) >= X_PAINT_TIMEOUT_MS;
}

static bool x_is_hide_cursor_on_win(x_t* x) {
    xwin_t* win = get_top_focus_win(x, false);
    if(win == NULL)
        return false;
    if(!win->xinfo->hide_cursor)
        return false;

    int32_t mx = x->cursor.cpos.x;
    int32_t my = x->cursor.cpos.y;

    if(mx < win->xinfo->wsr.x || mx > (win->xinfo->wsr.x + win->xinfo->wsr.w) ||
            my < win->xinfo->wsr.y || my > (win->xinfo->wsr.y + win->xinfo->wsr.h))
        return false;
    return true;
}

/*immediate cursor redraw for input latency: composites just the cursor
  into the scan-out buffer and posts a non-blocking dirty flush, so a
  mouse move shows up at event rate instead of one paced frame late.
  Runs inside the input IPC handler (a worker thread holding the server
  lock), so it must never block and must never touch the buffer or the
  ctrl dirty list while a push/flush is in flight - skipped then, and
  the frame path retries it via display->cursor_task.*/
bool x_cursor_redraw_now(x_t* x, uint32_t display_index) {
    x_display_t* display = &x->displays[display_index];
    if(!display->active || display->g == NULL ||
            display->flush_inflight ||
            display_busy(&display->display))
        return false;
    if((!x->show_cursor && !x->mouse_state.busy) ||
            x->current_display != display_index ||
            x_is_hide_cursor_on_win(x))
        return false;

    grect_t dirty[2];
    x_get_cursor_rect(x, &dirty[0], true);
    hide_cursor(x);
    x_get_cursor_rect(x, &dirty[1], false);
    refresh_cursor(x);
    display_set_dirty(&display->display, dirty, 2);
    display_flush(&display->display, false);
    return true;
}

#define X_WAIT_READY_MAX 4

void x_repaint(x_t* x, uint32_t display_index) {
    x_display_t* display = &x->displays[display_index];
    grect_t dirty_rects[X_REPAINT_DIRTY_MAX];
    uint32_t dirty_num = 0;
    bool cursor_hidden = false;
    bool cursor_refreshed = false;
    bool desktop_retry = false;
    bool paint_retry = false;
    grect_t cursor_old_rect;
    grect_t cursor_new_rect;

    if(display->g == NULL || !display->need_repaint)
        return;

    /*compositing writes straight into the scan-out dma now, so hold off
      while the fb daemon is still pushing the previous frame to the panel;
      otherwise it copies a half-drawn frame (tearing). need_repaint stays
      set so the frame is retried on the next step.*/
    if(display_busy(&display->display))
        return;

    if(!all_win_ready(x)) {
        /*wait a few frames only: a client stuck before its first update
          must not freeze the whole display*/
        if(display->wait_ready < X_WAIT_READY_MAX) {
            display->wait_ready++;
            return;
        }
    }
    display->wait_ready = 0;

    if(display->dirty) {
        /*a full rebuild paints the desktop over everything, so a window whose
          painter is mid-frame cannot simply be skipped the way an incremental
          repaint skips it - that would leave a hole. Hold the whole rebuild
          back until the painter publishes, with the same small budget
          all_win_ready uses; win_src_stable bounds the wait per window too, so
          an idle client cannot stall a rebuild forever.*/
        bool painting = false;
        xwin_t* w = x->win_head;
        while(w != NULL) {
            if(w->ready && w->xinfo != NULL && w->xinfo->visible &&
                    w->xinfo->display_index == display_index &&
                    !win_src_stable(x, w)) {
                painting = true;
                break;
            }
            w = w->next;
        }
        if(painting && display->paint_wait < X_WAIT_READY_MAX) {
            display->paint_wait++;
            return;
        }
    }
    display->paint_wait = 0;

    display->need_repaint = false;
    bool do_flush = false;

    if(display->cursor_task || x->mouse_state.busy) {
        display->cursor_task = false;
        do_flush = true;
    }	

    if((x->show_cursor || x->mouse_state.busy) && x->current_display == display_index) {
        if(!x_is_hide_cursor_on_win(x)) {
            x_get_cursor_rect(x, &cursor_old_rect, true);
            hide_cursor(x);
            cursor_hidden = true;
        }
    }

    /*erase the last drag outline by putting the saved scene band back; a
      full repaint rewrites that area anyway, so there the stale band is
      just dropped. Either way the outline is off the screen after this.*/
    if(x->current.drag_band_valid && x->current.drag_display == display_index) {
        if(!display->dirty) {
            graph_blt(x->current.drag_band, 0, 0,
                    x->current.drag_rect.w, x->current.drag_rect.h,
                    display->g, x->current.drag_rect.x, x->current.drag_rect.y,
                    x->current.drag_rect.w, x->current.drag_rect.h);
            x_repaint_add_dirty(display->g, dirty_rects, &dirty_num, &x->current.drag_rect);
            do_flush = true;
        }
        x->current.drag_band_valid = false;
    }

    if(display->dirty) {
        /* skip desktop drawing if fully covered by an opaque window
           (e.g. fullscreen window on top) */
        if(!covered_by_opaque_win(x, NULL, display_index, &display->desktop_rect)) {
            if(draw_desktop(x, display_index) == 0) {
                x_repaint_add_dirty(display->g, dirty_rects, &dirty_num, &display->desktop_rect);
                do_flush = true;
            }
            else {
                /*the xwm refused the draw: nothing was painted, so keep the
                  display dirty and retry next frame instead of flushing an
                  untouched (or worse, fallback-filled) scan-out region*/
                desktop_retry = true;
            }
        }
    }

    xwin_t* win = x->win_head;
    while(win != NULL) {
        if(win->ready && 
                win->xinfo->visible &&
                win->xinfo->display_index == display_index) {
            if(display->dirty) {
                win->dirty = true;
                win->shadow_valid = false; //the bands must be blended fresh
            }

            if(win->dirty || win->frame_dirty) {
                /* fully covered by an opaque window above: the covering
                   window paints over it later in this bottom-to-top pass,
                   so drawing it would be pure waste */
                if(win != x->current.win_drag &&
                                covered_by_opaque_win(x, win, display_index, &win->xinfo->winr)) {
                    win->dirty = false;
                    win->frame_dirty = false;
                    /*its area gets overwritten by the covering window, so
                      whatever shadow sat there is gone*/
                    win->shadow_valid = false;
                    /*the published frame is not going on screen, so hand the
                      buffer back now instead of leaving the client parked (or
                      locked out of its handoff buffer) until the accept
                      timeout. When the window is uncovered the rebuild reads
                      whatever is in the buffer then.*/
                    x_update_commit(x, win);
                }
                else if(!display->dirty && !win_src_stable(x, win)) {
                    /*mid-frame, and nothing below it changed: the scan-out still
                      holds the picture the last composite put there, so leave
                      the window alone (dirty stays set) and take its frame on a
                      later step instead of compositing half-drawn pixels*/
                    paint_retry = true;
                }
                else {
                    grect_t win_dirty;
                    if(draw_win(display->g, x, win, &win_dirty) == 0) {
                        if(!display->dirty && x->config.xwm_theme.shadow > 0)
                            refresh_shadows_above(x, win, &win_dirty);
                        x_repaint_add_dirty(display->g, dirty_rects, &dirty_num, &win_dirty);
                        do_flush = true;
                    }
                    /*draw_win is the last reader of the published buffer - it
                      also handed it to xwm for the decorations - so the painter
                      may have it back now, and not a moment earlier*/
                    x_update_commit(x, win);
                }
            }
        }
        win = win->next;
    }

    /*draw the drag frame as an overlay: save the scene band under the new
      outline rect, then let the xwm draw the outline into it. Only the
      old and new outline rects get flushed, so a drag step costs two band
      blits plus a small push instead of a whole-display repaint.*/
    if(x->current.win_drag != NULL && x->current.drag_state != 0 &&
            x->current.win_drag->xinfo->display_index == display_index) {
        grect_t r;
        get_drag_frame_rect(x, &r);
        /*the xwm draws the outline frameW pixels OUTSIDE the given rect
          (graph_frame at r inflated by frameW), so the band and the dirty
          region have to cover the inflated outline, or that outer ring is
          never restored and the frame trails*/
        int32_t wd = x->config.xwm_theme.frameW;
        if(wd <= 0)
            wd = 2;
        r.x -= wd;
        r.y -= wd;
        r.w += wd * 2;
        r.h += wd * 2;
        if(rect_clip_to_graph(display->g, &r) && r.w > 0 && r.h > 0) {
            if(x->current.drag_band == NULL ||
                    x->current.drag_band->w != r.w ||
                    x->current.drag_band->h != r.h) {
                if(x->current.drag_band != NULL)
                    graph_free(x->current.drag_band);
                x->current.drag_band = graph_new_shm(r.w, r.h);
            }
            if(x->current.drag_band != NULL) {
                graph_blt(display->g, r.x, r.y, r.w, r.h,
                        x->current.drag_band, 0, 0, r.w, r.h);
                x->current.drag_rect = r;
                x->current.drag_display = display_index;
                x->current.drag_band_valid = true;
                draw_drag_frame(x, display_index);
                x_repaint_add_dirty(display->g, dirty_rects, &dirty_num, &r);
                do_flush = true;
            }
        }
    }
    else if(x->current.drag_band != NULL &&
            x->current.drag_display == display_index) {
        /*the drag ended; the erase above already restored the scene*/
        graph_free(x->current.drag_band);
        x->current.drag_band = NULL;
        x->current.drag_band_valid = false;
    }

    if(x->current_display == display_index) {
        if(x->show_cursor || x->mouse_state.busy) {
            if(!x_is_hide_cursor_on_win(x)) {
                x_get_cursor_rect(x, &cursor_new_rect, false);
                refresh_cursor(x);
                cursor_refreshed = true;
            }
        }
    }

    if(cursor_hidden) {
        x_repaint_add_dirty(display->g, dirty_rects, &dirty_num, &cursor_old_rect);
    }
    if(cursor_refreshed) {
        x_repaint_add_dirty(display->g, dirty_rects, &dirty_num, &cursor_new_rect);
    }

    display->dirty = desktop_retry;
    /*a window skipped because its painter was mid-frame still owes its picture*/
    if(paint_retry)
        display->need_repaint = true;
    if(do_flush && dirty_num > 0) {
        /*tell the fb daemon what changed so it only pushes those areas to
          the scan-out buffer instead of the whole frame*/
        grect_t display_dirty[DISPLAY_DIRTY_MAX];
        uint32_t display_num = pack_dirty_rects(dirty_rects, dirty_num, display_dirty, DISPLAY_DIRTY_MAX);
        display_set_dirty(&display->display, display_dirty, display_num);
        /*the ctrl dirty list is a single slot owned by this frame until the
          flush lands: between this write and the flush completion the input
          handler's fast cursor flush must stay out of it (and out of the
          scan-out buffer), or the daemon would push the cursor rects
          instead of the frame rects - the erased drag outline then never
          reaches the panel and trails*/
        display->flush_inflight = true;
        /*defer the flush IPC until after the server lock is released to
          keep the locked compositing section as tight as possible*/
        display->pending_flush = true;
    }
}
