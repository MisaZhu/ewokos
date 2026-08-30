#ifndef XRENDER_H
#define XRENDER_H

/*compositing: desktop, window content and frame drawing*/

#include "xserver.h"

void draw_desktop(x_t* x, uint32_t display_index);
int draw_win(graph_t* disp_g, x_t* x, xwin_t* win, grect_t* out_dmg);

/*drag frame overlay: the outline rect, and drawing it (via the xwm) into
  the display scan-out buffer*/
void get_drag_frame_rect(x_t* x, grect_t* r);
void draw_drag_frame(x_t* x, uint32_t display_index);

/*a region was just repainted fresh; re-blend the shadow bands of the
  windows above that got wiped by it*/
void refresh_shadows_above(x_t* x, xwin_t* below, const grect_t* region);

#endif
