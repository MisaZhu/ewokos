/*the per-display repaint pipeline: dirty rect collection, compositing
  order, cursor overlay and the flush to the fb daemon*/
#include <string.h>
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

    if(x->cursor.drop) {
        /*what sits below the cursor got repainted behind its back (a
          direct-to-display window painted there): the saved backdrop is
          stale and must not be blended back onto the display. Free it and
          fall through so a fresh one is captured right away, otherwise
          refresh_cursor sees saved==NULL and the cursor stays invisible
          for a frame (flicker at the client's update rate).*/
        if(x->cursor.saved != NULL) {
            graph_free(x->cursor.saved);
            x->cursor.saved = NULL;
        }
        x->cursor.drop = false;
    }

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
    x->cursor.drop = false;
}

static bool all_win_ready(x_t* x) {
    xwin_t* win = x->win_head;
    while(win != NULL) {
        if(win->xinfo != NULL && win->xinfo->visible && !win->ready)
            return false;
        win = win->next;
    }
    return true;
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

#define X_WAIT_READY_MAX 4

void x_repaint(x_t* x, uint32_t display_index) {
    x_display_t* display = &x->displays[display_index];
    grect_t dirty_rects[X_REPAINT_DIRTY_MAX];
    uint32_t dirty_num = 0;
    bool cursor_hidden = false;
    bool cursor_refreshed = false;
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

    if(display->dirty) {
        /* skip desktop drawing if fully covered by an opaque window
           (e.g. fullscreen window on top) */
        if(!covered_by_opaque_win(x, NULL, display_index, &display->desktop_rect)) {
            draw_desktop(x, display_index);
            x_repaint_add_dirty(display->g, dirty_rects, &dirty_num, &display->desktop_rect);
            do_flush = true;
        }
    }

    xwin_t* win = x->win_head;
    while(win != NULL) {
        if(win->ready && 
                win->xinfo->visible &&
                win->xinfo->display_index == display_index) {
            if(display->dirty) {
                win->dirty = true;
                win->has_damage = false; //everything below it got repainted
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
                    win->has_damage = false;
                    /*its area gets overwritten by the covering window, so
                      whatever shadow sat there is gone*/
                    win->shadow_valid = false;
                }
                else {
                    grect_t win_dirty;
                    if(draw_win(display->g, x, win, &win_dirty) == 0) {
                        if(!display->dirty && x->config.xwm_theme.shadow > 0)
                            refresh_shadows_above(x, win, &win_dirty);
                        x_repaint_add_dirty(display->g, dirty_rects, &dirty_num, &win_dirty);
                        do_flush = true;
                    }
                    if(drag_win(display->g, x, win) == 0) {
                        x_repaint_add_dirty(display->g, dirty_rects, &dirty_num, &display->desktop_rect);
                        do_flush = true;
                    }
                }
            }
        }
        win = win->next;
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

    display->dirty = false;
    if(do_flush && dirty_num > 0) {
        /*tell the fb daemon what changed so it only pushes those areas to
          the scan-out buffer instead of the whole frame*/
        grect_t display_dirty[DISPLAY_DIRTY_MAX];
        uint32_t display_num = pack_dirty_rects(dirty_rects, dirty_num, display_dirty, DISPLAY_DIRTY_MAX);
        display_set_dirty(&display->display, display_dirty, display_num);
        /*defer the flush IPC until after ipc_enable() to keep the
          ipc_disable() critical section as tight as possible*/
        display->pending_flush = true;
    }
}
