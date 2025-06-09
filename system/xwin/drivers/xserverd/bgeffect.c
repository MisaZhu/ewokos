#include "bgeffect.h"

void x_bg_effect(graph_t* disp_g, xwin_t* win, uint32_t bg_effect) {
    if(bg_effect == BG_EFFECT_NONE)
        return;

    graph_blt_alpha(win->g_buf, 0, 0, 
        win->xinfo->wsr.w,
        win->xinfo->wsr.h,
        disp_g,
        win->xinfo->wsr.x,
        win->xinfo->wsr.y,
        win->xinfo->wsr.w,
        win->xinfo->wsr.h, 0x88);

    switch(bg_effect) {
    case BG_EFFECT_TRANSPARENT:
        return;
    case BG_EFFECT_DOT:
        graph_draw_dot_pattern(disp_g, 
            win->xinfo->wsr.x,
            win->xinfo->wsr.y,
            win->xinfo->wsr.w,
            win->xinfo->wsr.h, 
            0x88FFFFFF,
            0x88444444,
            1);
        return;
    case BG_EFFECT_GLASS:
        graph_glass(disp_g, 
            win->xinfo->wsr.x,
            win->xinfo->wsr.y,
            win->xinfo->wsr.w,
            win->xinfo->wsr.h,
            2);
        return;
    }
}