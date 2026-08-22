#ifndef XRENDER_H
#define XRENDER_H

/*compositing: desktop, window content and frame drawing*/

#include "xserver.h"

void draw_desktop(x_t* x, uint32_t display_index);
int draw_win(graph_t* disp_g, x_t* x, xwin_t* win, grect_t* out_dmg);
int drag_win(graph_t* disp_g, x_t* x, xwin_t* win);

/*a region was just repainted fresh; re-blend the shadow bands of the
  windows above that got wiped by it*/
void refresh_shadows_above(x_t* x, xwin_t* below, const grect_t* region);

#endif
