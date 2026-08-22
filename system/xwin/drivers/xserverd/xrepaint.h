#ifndef XREPAINT_H
#define XREPAINT_H

/*the per-display repaint pipeline: dirty rect collection, compositing
  order and cursor overlay*/

#include "xserver.h"

void x_repaint(x_t* x, uint32_t display_index);

/*save/restore of the pixels under the cursor; also used by the input
  code when the cursor shape changes*/
void hide_cursor(x_t* x);

#endif
