/*compositing: desktop drawing, per-window content preparation and the
  blits that put window pictures onto the display graph*/
#include <string.h>
#include <ewoksys/ipc.h>
#include <x/xwm.h>
#include "xrender.h"
#include "xwin.h"

static bool top_proc(x_t* x, xwin_t* win) {
    if(x->win_focus == NULL)
        return false;
    if(win->from_main_pid == x->win_focus->from_main_pid)
        return true;
    return false;
}

static void win_mark_frame_dirty(x_t* x, xwin_t* win) {
    x_display_t *display = &x->displays[win->xinfo->display_index];

    /*the background effect mixes the desktop into the frame of an unfocused
      window, so its frame has to be built again whenever the content
      changed. Without such an effect the frame keeps its picture.
      A frame-direct window gets no background effect at all (its frame is
      the display itself), same as a fullscreen one.*/
    if(win->dirty && !win->xinfo->focused && !win->frame_direct &&
            x->config.xwm_theme.bgEffect != 0 &&
            (win->xinfo->style & XWIN_STYLE_NO_BG_EFFECT) == 0) {
        win->frame_dirty = true;
        return;
    }

    /*a desktop repaint only invalidates the frames that blend with what is
      below them; a frame drawn opaquely still holds a valid picture*/
    if(display->dirty && need_repaint_frame(x, win))
        win->frame_dirty = true;
}

/*the workspace area gets fully overwritten by the blt in
  prepare_win_content, so only the decoration ring around it has to be
  reset for the alpha drawing of xwm*/
static void clear_frame_ring(xwin_t* win) {
    graph_t* g = win->frame_g;
    if(g == NULL)
        return;
    /*the ring of a frame-direct window lives on the display: wiping it
      would erase whatever sits around the workspace there*/
    if(win->frame_direct)
        return;

    grect_t bounds = {0, 0, g->w, g->h};
    grect_t ws = {win->xinfo->wsr.x - win->xinfo->winr.x,
            win->xinfo->wsr.y - win->xinfo->winr.y,
            win->xinfo->wsr.w, win->xinfo->wsr.h};

    if(!grect_insect(&bounds, &ws) ||
            (ws.x == 0 && ws.y == 0 && ws.w == g->w && ws.h == g->h)) {
        graph_clear(g, 0);
        return;
    }

    graph_set(g, 0, 0, g->w, ws.y, 0); //top
    graph_set(g, 0, ws.y + ws.h, g->w, g->h - ws.y - ws.h, 0); //bottom
    graph_set(g, 0, ws.y, ws.x, ws.h, 0); //left
    graph_set(g, ws.x + ws.w, ws.y, g->w - ws.x - ws.w, ws.h, 0); //right
}

/*ws_dmg: damaged area of the workspace, NULL means all of it*/
static void prepare_win_content(x_t* x, xwin_t* win, const grect_t* ws_dmg) {
    x_display_t *display = &x->displays[win->xinfo->display_index];
    if(display->g == NULL)
        return;

    if(win->frame_g == NULL)
        return;

    if(win->frame_dirty) {
        clear_frame_ring(win);
        ws_dmg = NULL; //the whole frame is being rebuilt
    }

    /*two kinds of windows need their content inside frame_g:
      - the background effect mixes the desktop into the whole window, so
        the blended picture has to be rebuilt there on every change;
      - a translucent frame (rounded corners) is drawn on top of the
        workspace, so xwm has to blend it over a fresh content copy.
      Every other workspace is composited straight from the workspace
      snapshot in draw_win, so this copy stays untouched. A frame-direct
      window composites the snapshot straight into the display itself.*/
    if(!win->frame_direct &&
            (win_bg_effect_active(x, win) || frame_cuts_ws(x, win)) &&
            (win->dirty || win->frame_dirty)) {
        graph_t* g = win->ws_g_buffer;
        int32_t ox = win->xinfo->wsr.x - win->xinfo->winr.x;
        int32_t oy = win->xinfo->wsr.y - win->xinfo->winr.y;
        //klog("win title: %s win->dirty: %d win->frame_dirty: %d\n", win->xinfo->title, win->dirty, win->frame_dirty);
        if(ws_dmg != NULL) {
            graph_blt(g, ws_dmg->x, ws_dmg->y, ws_dmg->w, ws_dmg->h,
                    win->frame_g,
                    ox + ws_dmg->x, oy + ws_dmg->y,
                    ws_dmg->w, ws_dmg->h);
        }
        else {
            graph_blt(g, 0, 0, g->w, g->h,
                    win->frame_g, ox, oy,
                    win->xinfo->wsr.w,
                    win->xinfo->wsr.h);
        }
    }

    if(!win->frame_dirty)
        return;

    /*a frameless window only visits xwm for the background effect;
      without it there are no decorations to draw, so the DRAW_FRAME
      round trip is skipped. A fullscreen window is edge to edge and has
      no decorations at all, so it gets the same treatment.*/
    if((win->xinfo->style & XWIN_STYLE_NO_FRAME) != 0 &&
            !win_bg_effect_active(x, win))
        return;
    if(win->xinfo->state == XWIN_STATE_FULL_SCREEN &&
            !win_bg_effect_active(x, win))
        return;

    if(!check_xwm(x))
        return;

    /*a frame-direct window hands the display shm over as its frame: the
      background effect would read it back and blend onto the same buffer
      in one go. It stays decoration-only, like a fullscreen window.*/
    xinfo_t frame_info;
    xinfo_t* info = win->xinfo;
    if(win->frame_direct && win_bg_effect_active(x, win)) {
        memcpy(&frame_info, win->xinfo, sizeof(xinfo_t));
        frame_info.style |= XWIN_STYLE_NO_BG_EFFECT;
        info = &frame_info;
    }

    //klog("win title: %s win->frame_dirty: %d\n", win->xinfo->title, win->frame_dirty);
    proto_t in;
    PF->format(&in, "i,i,i,m",
        (ewokos_addr_t)display->g_shm_id,
        (ewokos_addr_t)display->g->w,
        (ewokos_addr_t)display->g->h,
        info, sizeof(xinfo_t));

    if(top_proc(x, win))
        PF->addi(&in, 1); //top win
    else
        PF->addi(&in, 0);

    ipc_call_wait(x->xwm_pid, XWM_CNTL_DRAW_FRAME, &in);
    //ipc_call(x->xwm_pid, XWM_CNTL_DRAW_FRAME, &in, NULL);
    PF->clear(&in);
}

static void draw_init_desktop(x_t* x, x_display_t *display) {
    graph_draw_dot_pattern(display->g, 0, 0, display->g->w, display->g->h,
            0xff888888, 0xffffffff, 2, 1);
    
    if(x->config.logo != NULL) {
        graph_blt_alpha(x->config.logo, 0, 0, x->config.logo->w, x->config.logo->h,
            display->g, 
            (display->g->w - x->config.logo->w)/2,
            (display->g->h - x->config.logo->h)/2,
            x->config.logo->w,
            x->config.logo->h,
            0xff);
    }
}

void draw_desktop(x_t* x, uint32_t display_index) {
    x_display_t *display = &x->displays[display_index];
    if(display->g == NULL)
        return;

    if(!check_xwm(x)) {
        draw_init_desktop(x, display);
        return;
    }	
    else if(x->config.logo != NULL) {
        graph_free(x->config.logo);
        x->config.logo = NULL;
    }

    proto_t in;
    PF->format(&in, "i,i,i",
        (ewokos_addr_t)display->g_shm_id,
        (ewokos_addr_t)display->g->w,
        (ewokos_addr_t)display->g->h);

    int res = ipc_call_wait(x->xwm_pid, XWM_CNTL_DRAW_DESKTOP, &in);
    PF->clear(&in);
    if(res != 0)
        graph_fill_rect(display->g, 0, 0, display->g->w, display->g->h, 0xff000000);
}

static void draw_drag_frame(x_t* xp, uint32_t display_index) {
    x_display_t *display = &xp->displays[display_index];
    if(display->g == NULL)
        return;

    int x = xp->current.win_drag->xinfo->winr.x;
    int y = xp->current.win_drag->xinfo->winr.y;
    int w = xp->current.win_drag->xinfo->winr.w - xp->config.xwm_theme.shadow;
    int h = xp->current.win_drag->xinfo->winr.h - xp->config.xwm_theme.shadow;

    if(xp->current.drag_state == X_win_DRAG_MOVE)  {
        x += xp->current.pos_delta.x;
        y += xp->current.pos_delta.y;
    }
    else if(xp->current.drag_state == X_win_DRAG_RESIZE)  {
        w += xp->current.pos_delta.x;
        h += xp->current.pos_delta.y;
    }

    grect_t r = {x, y, w, h};

    proto_t in;
    PF->format(&in, "i,i,i,m",
        (ewokos_addr_t)display->g_shm_id,
        (ewokos_addr_t)display->g->w,
        (ewokos_addr_t)display->g->h,
        &r, sizeof(grect_t));

    if(check_xwm(xp))
        ipc_call_wait(xp->xwm_pid, XWM_CNTL_DRAW_DRAG_FRAME, &in);
    PF->clear(&in);
}

/*blit the part of area that falls inside dmg: dmg and area are in frame
  coordinates, the result lands at win_x/win_y on the display*/
static inline void blit_win_area(graph_t* g, graph_t* disp_g, int32_t win_x, int32_t win_y,
        const grect_t* dmg, const grect_t* area, bool alpha) {
    grect_t d = *dmg;
    if(!grect_insect(area, &d) || d.w <= 0 || d.h <= 0)
        return;

    if(alpha)
        graph_blt_alpha(g, d.x, d.y, d.w, d.h,
                disp_g, win_x + d.x, win_y + d.y, d.w, d.h, 0xff);
    else
        graph_blt(g, d.x, d.y, d.w, d.h,
                disp_g, win_x + d.x, win_y + d.y, d.w, d.h);
}

/*blit the rect d (frame coordinates) of the window onto the display,
  sourcing the pixels split up: the workspace part comes straight from
  the workspace snapshot (frame_g never received it), everything around
  it comes from frame_g where xwm put the decorations. While the
  background effect blends the window or a translucent frame is drawn
  over the workspace the whole picture only exists in frame_g, so those
  cases fall back to a single source.*/
static void blit_win_part(x_t* x, xwin_t* win, graph_t* disp_g,
        const grect_t* d, bool alpha) {
    if(d->w <= 0 || d->h <= 0)
        return;

    graph_t* ring = win->frame_g;
    if(ring == NULL)
        return;

    int32_t win_x = win->xinfo->winr.x;
    int32_t win_y = win->xinfo->winr.y;

    if(win_bg_effect_active(x, win) || frame_cuts_ws(x, win) ||
            win->ws_g_buffer == NULL) {
        if(alpha)
            graph_blt_alpha(ring, d->x, d->y, d->w, d->h,
                    disp_g, win_x + d->x, win_y + d->y, d->w, d->h, 0xff);
        else
            graph_blt(ring, d->x, d->y, d->w, d->h,
                    disp_g, win_x + d->x, win_y + d->y, d->w, d->h);
        return;
    }

    graph_t* ws = win->ws_g_buffer;
    int32_t ox = win->xinfo->wsr.x - win->xinfo->winr.x;
    int32_t oy = win->xinfo->wsr.y - win->xinfo->winr.y;

    int32_t d1 = d->x, d2 = d->x + d->w;
    int32_t e1 = d->y, e2 = d->y + d->h;
    int32_t w1 = ox, w2 = ox + (int32_t)win->xinfo->wsr.w;
    int32_t n1 = oy, n2 = oy + (int32_t)win->xinfo->wsr.h;

    /*the part of d inside the workspace: straight from the snapshot*/
    int32_t ix1 = d1 > w1 ? d1 : w1;
    int32_t iy1 = e1 > n1 ? e1 : n1;
    int32_t ix2 = d2 < w2 ? d2 : w2;
    int32_t iy2 = e2 < n2 ? e2 : n2;
    if(ix1 < ix2 && iy1 < iy2) {
        if(alpha)
            graph_blt_alpha(ws, ix1 - ox, iy1 - oy, ix2 - ix1, iy2 - iy1,
                    disp_g, win_x + ix1, win_y + iy1, ix2 - ix1, iy2 - iy1, 0xff);
        else
            graph_blt(ws, ix1 - ox, iy1 - oy, ix2 - ix1, iy2 - iy1,
                    disp_g, win_x + ix1, win_y + iy1, ix2 - ix1, iy2 - iy1);
    }

    /*the parts of d outside the workspace: up to four bands from frame_g*/
    grect_t bands[4];
    uint32_t num = 0;
    int32_t y0 = e1 > n1 ? e1 : n1;
    int32_t y1 = e2 < n2 ? e2 : n2;
    if(e1 < n1) //top
        bands[num++] = (grect_t){d1, e1, d2 - d1, (n1 < e2 ? n1 : e2) - e1};
    if(n2 < e2) //bottom
        bands[num++] = (grect_t){d1, n2 > e1 ? n2 : e1, d2 - d1, e2 - (n2 > e1 ? n2 : e1)};
    if(y0 < y1) {
        if(d1 < w1) //left
            bands[num++] = (grect_t){d1, y0, (w1 < d2 ? w1 : d2) - d1, y1 - y0};
        if(w2 < d2) //right
            bands[num++] = (grect_t){w2 > d1 ? w2 : d1, y0, d2 - (w2 > d1 ? w2 : d1), y1 - y0};
    }
    for(uint32_t i = 0; i < num; i++) {
        grect_t* b = &bands[i];
        if(b->w <= 0 || b->h <= 0)
            continue;
        if(alpha)
            graph_blt_alpha(ring, b->x, b->y, b->w, b->h,
                    disp_g, win_x + b->x, win_y + b->y, b->w, b->h, 0xff);
        else
            graph_blt(ring, b->x, b->y, b->w, b->h,
                    disp_g, win_x + b->x, win_y + b->y, b->w, b->h);
    }
}

/*out_dmg gets the area of disp_g the window actually touched*/
int draw_win(graph_t* disp_g, x_t* x, xwin_t* win, grect_t* out_dmg) {
    if(win->ws_direct) {
        /*the client paints straight into the display graph: never touch
          the pixels here, just declare the region dirty so the fb daemon
          pushes it and whatever sits above the window gets repainted by
          the normal bottom-to-top pass*/
        memcpy(out_dmg, &win->xinfo->winr, sizeof(grect_t));
        win->dirty = false;
        win->frame_dirty = false;
        win->has_damage = false;
        return 0;
    }

    if(win->frame_direct) {
        /*frame_g is the display graph: the decoration was painted there by
          xwm (prepare_win_content sends the display shm as the frame) and
          the workspace snapshot goes straight into the scan-out buffer.
          The usual frame_g-to-display blit would be a self-copy.*/
        win_mark_frame_dirty(x, win);

        grect_t ws_dmg = win->damage;
        bool has_dmg = win->has_damage && !win->frame_dirty;

        prepare_win_content(x, win, has_dmg ? &ws_dmg : NULL);

        if(win->ws_g_buffer != NULL) {
            if(has_dmg) {
                graph_blt(win->ws_g_buffer, ws_dmg.x, ws_dmg.y,
                        ws_dmg.w, ws_dmg.h,
                        disp_g,
                        win->xinfo->wsr.x + ws_dmg.x,
                        win->xinfo->wsr.y + ws_dmg.y,
                        ws_dmg.w, ws_dmg.h);
                out_dmg->x = win->xinfo->wsr.x + ws_dmg.x;
                out_dmg->y = win->xinfo->wsr.y + ws_dmg.y;
                out_dmg->w = ws_dmg.w;
                out_dmg->h = ws_dmg.h;
            }
            else {
                graph_blt(win->ws_g_buffer, 0, 0,
                        win->xinfo->wsr.w, win->xinfo->wsr.h,
                        disp_g,
                        win->xinfo->wsr.x, win->xinfo->wsr.y,
                        win->xinfo->wsr.w, win->xinfo->wsr.h);
                memcpy(out_dmg, &win->xinfo->winr, sizeof(grect_t));
            }
        }
        /*the decoration redraw touched the ring around the workspace*/
        if(win->frame_dirty)
            memcpy(out_dmg, &win->xinfo->winr, sizeof(grect_t));

        win->dirty = false;
        win->frame_dirty = false;
        win->has_damage = false;
        return 0;
    }

    win_mark_frame_dirty(x, win);

    grect_t ws_dmg = win->damage;
    bool has_dmg = win->has_damage && !win->frame_dirty;

    prepare_win_content(x, win, has_dmg ? &ws_dmg : NULL);

    grect_t dmg; //damaged area in frame_g coordinates
    if(has_dmg) {
        dmg.x = ws_dmg.x + win->xinfo->wsr.x - win->xinfo->winr.x;
        dmg.y = ws_dmg.y + win->xinfo->wsr.y - win->xinfo->winr.y;
        dmg.w = ws_dmg.w;
        dmg.h = ws_dmg.h;
    }
    else {
        dmg.x = 0;
        dmg.y = 0;
        dmg.w = win->xinfo->winr.w;
        dmg.h = win->xinfo->winr.h;
    }

    /*unless the background effect or a translucent frame keeps the whole
      picture inside frame_g, that graph only holds the decorations here:
      the workspace pixels are composited straight from the workspace
      snapshot. blit_win_part does that split for every rect handed to
      it.*/
    graph_t* g = win->frame_g;
    if(g != NULL) {
        grect_t bounds = {0, 0, g->w, g->h};
        if(!grect_insect(&bounds, &dmg)) {
            dmg.w = 0;
            dmg.h = 0;
        }
        else if(win->xinfo->alpha) {
            /*the window content itself is translucent, blend the whole window*/
            blit_win_part(x, win, disp_g, &dmg, true);
        }
        else if(x->config.xwm_theme.shadow > 0 &&
                !x->config.xwm_theme.frameAlpha) {
            /*everything is opaque except the shadow, and the shadow only
              lives in the strips right of and below the workspace: copy the
              rest with a plain blit and blend just those strips*/
            int32_t s_right = win->xinfo->winr.w -
                    ((win->xinfo->wsr.x - win->xinfo->winr.x) + win->xinfo->wsr.w);
            int32_t s_bottom = win->xinfo->winr.h -
                    ((win->xinfo->wsr.y - win->xinfo->winr.y) + win->xinfo->wsr.h);
            if(s_right < 0)
                s_right = 0;
            if(s_bottom < 0)
                s_bottom = 0;

            grect_t inner = {0, 0, g->w - s_right, g->h - s_bottom};
            grect_t right = {g->w - s_right, 0, s_right, g->h};
            grect_t bottom = {0, g->h - s_bottom, g->w - s_right, s_bottom};

            grect_t d = dmg;
            if(grect_insect(&inner, &d))
                blit_win_part(x, win, disp_g, &d, false);

            /*the bands already sit blended on the display: blending them
              again would put shadow on shadow and darken them further, so
              they are only touched again once what is below them was
              repainted (which invalidates the flag) or the window moved*/
            bool bands_ok = win->shadow_valid &&
                    memcmp(&win->shadow_rect, &win->xinfo->winr, sizeof(grect_t)) == 0;
            if(!bands_ok) {
                /*the bands are missing on the display, so they get blended
                  whole no matter what the damaged area of this draw is*/
                grect_t whole = {0, 0, g->w, g->h};
                blit_win_area(g, disp_g, win->xinfo->winr.x, win->xinfo->winr.y,
                        &whole, &right, true);
                blit_win_area(g, disp_g, win->xinfo->winr.x, win->xinfo->winr.y,
                        &whole, &bottom, true);
                win->shadow_valid = true;
                memcpy(&win->shadow_rect, &win->xinfo->winr, sizeof(grect_t));
            }
        }
        else if(frame_cuts_ws(x, win)) {
            /*for themed alpha frames the whole picture gets rebuilt in
              frame_g (the rounded corners are cut into the content there),
              so only the border ring needs blending; copy the middle
              opaquely and blend the four edges. The edge width follows the
              larger of frame width and round radius. Edge-to-edge windows
              (maximized/fullscreen) have no translucent frame pixels and
              must not take this path: their frame_g holds no content.*/
            int32_t edge = (int32_t)x->config.xwm_theme.frameW;
            if((int32_t)x->config.xwm_theme.round > edge)
                edge = (int32_t)x->config.xwm_theme.round;
            if(edge > g->w/2)
                edge = g->w/2;
            if(edge > g->h/2)
                edge = g->h/2;

            if(edge > 0) {
                int32_t wx = win->xinfo->winr.x;
                int32_t wy = win->xinfo->winr.y;

                grect_t top = {0, 0, g->w, edge};
                grect_t bottom = {0, g->h - edge, g->w, edge};
                grect_t left = {0, edge, edge, g->h - 2*edge};
                grect_t right = {g->w - edge, edge, edge, g->h - 2*edge};
                grect_t mid = {edge, edge, g->w - 2*edge, g->h - 2*edge};

                blit_win_area(g, disp_g, wx, wy, &dmg, &top, true);
                blit_win_area(g, disp_g, wx, wy, &dmg, &bottom, true);
                blit_win_area(g, disp_g, wx, wy, &dmg, &left, true);
                blit_win_area(g, disp_g, wx, wy, &dmg, &right, true);
                grect_t d = dmg;
                if(grect_insect(&mid, &d))
                    blit_win_part(x, win, disp_g, &d, false);
            }
            else {
                blit_win_part(x, win, disp_g, &dmg, false);
            }
        }
        else {
            blit_win_part(x, win, disp_g, &dmg, false);
        }
    }

    out_dmg->x = win->xinfo->winr.x + dmg.x;
    out_dmg->y = win->xinfo->winr.y + dmg.y;
    out_dmg->w = dmg.w;
    out_dmg->h = dmg.h;

    win->dirty = false;
    win->frame_dirty = false;
    win->has_damage = false;
    return 0;
}

int drag_win(graph_t* disp_g, x_t* x, xwin_t* win) {
    if(x->current.win_drag == win &&
            (win->xinfo->style & XWIN_STYLE_NO_FRAME) == 0 &&
            win->xinfo->state != XWIN_STATE_MAX &&
            win->xinfo->state != XWIN_STATE_FULL_SCREEN) {
        draw_drag_frame(x, win->xinfo->display_index);
        return 0;
    }
    return -1;
}

/*a region was just repainted fresh; where it reaches into the shadow bands
  of the windows above, their shadow has been wiped away or sits blended on
  top of stale pixels. Repair exactly those parts while the background is
  still pristine: re-blending a whole band later would double the shadow on
  the parts that were left alone. Not needed while the whole display is
  being rebuilt: that pass repaints every window bottom to top, so each one
  blends its bands onto a fresh background itself.*/
void refresh_shadows_above(x_t* x, xwin_t* below, const grect_t* region) {
    if(x->config.xwm_theme.shadow <= 0)
        return;
    x_display_t* display = &x->displays[below->xinfo->display_index];
    xwin_t* w = below->next;
    while(w != NULL) {
        if(w->ready && w->xinfo != NULL && w->xinfo->visible &&
                w->xinfo->display_index == below->xinfo->display_index &&
                w->frame_g != NULL && !w->frame_direct) {
            int32_t s_right = w->xinfo->winr.w -
                    ((w->xinfo->wsr.x - w->xinfo->winr.x) + w->xinfo->wsr.w);
            int32_t s_bottom = w->xinfo->winr.h -
                    ((w->xinfo->wsr.y - w->xinfo->winr.y) + w->xinfo->wsr.h);
            if(s_right < 0)
                s_right = 0;
            if(s_bottom < 0)
                s_bottom = 0;

            if(s_right > 0 || s_bottom > 0) {
                bool bands_ok = w->shadow_valid &&
                        memcmp(&w->shadow_rect, &w->xinfo->winr, sizeof(grect_t)) == 0;
                if(bands_ok) {
                    /*the untouched parts of the bands are still fine: fix
                      only what the fresh region wiped*/
                    grect_t d = *region;
                    d.x -= w->xinfo->winr.x;
                    d.y -= w->xinfo->winr.y;
                    grect_t right = {w->frame_g->w - s_right, 0, s_right, w->frame_g->h};
                    grect_t bottom = {0, w->frame_g->h - s_bottom,
                            w->frame_g->w - s_right, s_bottom};
                    blit_win_area(w->frame_g, display->g,
                            w->xinfo->winr.x, w->xinfo->winr.y,
                            &d, &right, true);
                    blit_win_area(w->frame_g, display->g,
                            w->xinfo->winr.x, w->xinfo->winr.y,
                            &d, &bottom, true);
                }
                else {
                    /*the bands were never blended for this placement: the
                      window has to paint them whole on its own draw*/
                    w->dirty = true;
                    w->has_damage = false;
                }
            }
        }
        w = w->next;
    }
}
