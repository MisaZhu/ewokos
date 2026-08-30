/*mouse/touch and IME input handling: pointer tracking, window hit
  testing, drag move/resize gestures and event routing to windows*/
#include <stdlib.h>
#include <ewoksys/basic_math.h>
#include "xinput.h"
#include "xwin.h"
#include "xrepaint.h" //hide_cursor

static int get_win_frame_pos(x_t* x, xwin_t* win) {
    if((win->xinfo->style & XWIN_STYLE_NO_FRAME) != 0)
        return -1;

    int res = -1;
    if(check_in_rect(x->cursor.cpos.x, x->cursor.cpos.y, &win->r_close))
        res = FRAME_R_CLOSE;
    else if(check_in_rect(x->cursor.cpos.x, x->cursor.cpos.y, &win->r_min))
        res = FRAME_R_MIN;
    else if(check_in_rect(x->cursor.cpos.x, x->cursor.cpos.y, &win->r_max))
        res = FRAME_R_MAX;
    else if(check_in_rect(x->cursor.cpos.x, x->cursor.cpos.y, &win->r_title))
        res = FRAME_R_TITLE;
    else if(check_in_rect(x->cursor.cpos.x, x->cursor.cpos.y, &win->r_resize))
        res = FRAME_R_RESIZE;
    return res;
}

xwin_t* get_mouse_owner(x_t* x, int* win_frame_pos) {
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
        int pos = get_win_frame_pos(x, win);
        if(pos >= 0) {
            if(win_frame_pos != NULL)
                *win_frame_pos = pos;
            return win;
        }
        if(check_in_rect(x->cursor.cpos.x, x->cursor.cpos.y, &win->xinfo->wsr))
            return win;
        win = win->prev;
    }
    return NULL;
}

int x_cursor_set_busy(x_t* x, bool busy) {
    if(x->mouse_state.busy == busy)
        return 0;

    hide_cursor(x);
    if(x->cursor.saved != NULL) {
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
    //refresh_cursor(x);
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
        else if(win->xinfo->style & XWIN_STYLE_NO_FRAME) {
            x->current.old_pos.x = x->cursor.cpos.x;
            x->current.old_pos.y = x->cursor.cpos.y;
            x->current.win_drag = win;
        }
    }
    else if(ev->state ==  MOUSE_STATE_DRAG) {
        if(win->xinfo->state != XWIN_STATE_MAX &&
                win->xinfo->state != XWIN_STATE_FULL_SCREEN) {
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
            return;
        }

        if(x->current.win_drag == win &&
                x->current.drag_state != 0 &&
                win->xinfo->state != XWIN_STATE_MAX &&
                win->xinfo->state != XWIN_STATE_FULL_SCREEN) {
            ev->type = XEVT_WIN;
            ev->value.window.v0 =  x->current.pos_delta.x;
            ev->value.window.v1 =  x->current.pos_delta.y;
            if(x->current.drag_state == X_win_DRAG_RESIZE) {
                if(x->current.pos_delta.x != 0 ||
                    x->current.pos_delta.y != 0 ) {
                    ev->value.window.event = XEVT_WIN_RESIZE;
                    /*graph_free(win->ws_g);
                    shmdt(win->ws_g_shm);
                    win->ws_g = NULL;
                    win->ws_g_shm = NULL;

                    if(win->ws_g_buffer != NULL) {
                        graph_free(win->ws_g_buffer);
                        win->ws_g_buffer = NULL;
                    }
                    */
                }
            }
            else if(x->current.drag_state == X_win_DRAG_MOVE) {
                ev->value.window.event = XEVT_WIN_MOVE;
            }
            x->current.pos_delta.x = 0;
            x->current.pos_delta.y = 0;
        }
        else if(abs_32(ev->value.mouse.from_x - ev->value.mouse.x) < 6 &&
                abs_32(ev->value.mouse.from_y - ev->value.mouse.y) < 6) {
            x_push_event(x, win, ev);
            ev->state = MOUSE_STATE_CLICK;
        }

        if(ev->state == MOUSE_STATE_CLICK) {
            if(pos == FRAME_R_CLOSE) { //window close
                ev->type = XEVT_WIN;
                ev->value.window.event = XEVT_WIN_CLOSE;
                //win->xinfo->visible = false;
                //x_dirty(x);
            }
            else if(pos == FRAME_R_MAX) {
                ev->type = XEVT_WIN;
                ev->value.window.event = XEVT_WIN_MAX;
            }
        }
        x->current.win_drag = NULL;
        x->current.drag_state = 0;
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
    if(x->mouse_state.state == MOUSE_STATE_NONE && ev->state == MOUSE_STATE_UP)
        return 0;

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
        //else if(ev->value.mouse.from_x != ev->value.mouse.x ||
            //		ev->value.mouse.from_y != ev->value.mouse.y ||
        else if(abs(x->mouse_state.last_pos.x - ev->value.mouse.x) > 3 ||
                abs(x->mouse_state.last_pos.y - ev->value.mouse.y) > 3 ||
                    x->mouse_state.state == MOUSE_STATE_DRAG) {
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

    int pos = -1;
    xwin_t* win = NULL;
    if(x->current.win_drag != NULL)
        win = x->current.win_drag;
    else {
        win = get_mouse_owner(x, &pos);
    }

    if(win != NULL) {
        x_cursor_set_busy(x, win->busy);
        mouse_xwin_handle(x, win, pos, ev);
    }
    else {
        x_cursor_set_busy(x, false);
        if(ev->state ==  MOUSE_STATE_DOWN)
            x_unfocus(x);
    }

    /*redraw the cursor right away (cheap composite + non-blocking flush)
      so the pointer tracks at event rate; waiting for the next paced
      frame would show every move a frame period late. When it is skipped
      (daemon mid-push / frame flush in flight) cursor_task stays set and
      the frame path draws it instead.*/
    if(x_cursor_redraw_now(x, x->current_display))
        display->cursor_task = false;

    return 0;
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

void handle_input(x_t* x, int32_t from_pid, xevent_t* ev) {
    if(ev->type == XEVT_IM) {
        im_handle(x, from_pid, ev);
    }
    else if(ev->type == XEVT_MOUSE) {
        mouse_handle(x, ev);
        x_repaint_req(x, x->current_display);
    }
}
