#ifndef X_DEV_BG_EFFECT_H
#define X_DEV_BG_EFFECT_H

#include <xserver.h>
#include <graph/graph.h>

enum {
    BG_EFFECT_NONE = 0,
    BG_EFFECT_TRANSPARENT,
    BG_EFFECT_DOT,
    BG_EFFECT_GLASS
};

void x_bg_effect(graph_t* disp_g, xwin_t* win, uint32_t bg_effect);

#endif