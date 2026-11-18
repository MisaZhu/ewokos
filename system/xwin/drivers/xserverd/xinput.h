#ifndef XINPUT_H
#define XINPUT_H

/*mouse/touch and IME input handling*/

#include "xserver.h"

/*areas of a window frame the pointer can hit*/
enum {
	FRAME_R_TITLE = 0,
	FRAME_R_CLOSE,
	FRAME_R_MIN,
	FRAME_R_MAX,
	FRAME_R_RESIZE
};

/*the topmost window under the pointer, win_frame_pos gets the frame
  area hit (or -1 for the workspace part)*/
xwin_t* get_mouse_owner(x_t* x, int* win_frame_pos);
int x_cursor_set_busy(x_t* x, bool busy);

/*entry for X_DCNTL_INPUT events*/
void handle_input(x_t* x, int32_t from_pid, xevent_t* ev);

/*on-demand MT pipeline counters for the "stat" devcmd (no logging)*/
void x_mt_diag_stats(uint32_t* frames, uint32_t* rel_only,
        uint32_t* delivered, uint32_t* dropped,
        int* last_n, char* last_edges, size_t edges_size);

#endif
