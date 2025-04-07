#include <mouse/mouse.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

static int8_t _mouse_down = 0;
#define MOUSE_EVT_MAX 4

static void mouse_evt(int8_t bt, int8_t rx, int8_t ry, mouse_evt_t* evt) {
	evt->state = MOUSE_STATE_MOVE;
	evt->button = MOUSE_BUTTON_NONE;
	evt->rx = rx;
	evt->ry = ry;

	if(bt == 2) 
		evt->button = MOUSE_BUTTON_LEFT;
	else if(bt == 3) 
		evt->button = MOUSE_BUTTON_MID;
	else if(bt == 4) 
		evt->button = MOUSE_BUTTON_RIGHT;
	else if(bt == 8) {
		if(rx > 0)
			evt->button = MOUSE_BUTTON_SCROLL_UP;
		else
			evt->button = MOUSE_BUTTON_SCROLL_DOWN;
		evt->rx = 0;
		evt->ry = 0;
	}

	if((bt > 1 && bt != 8)||
			(bt == 0 && _mouse_down == 1)) {//down
		evt->state = MOUSE_STATE_DOWN;
		_mouse_down = 1;
	}
	else if(bt == 1) {//up
		evt->state = MOUSE_STATE_UP;
		_mouse_down = 0;
	}
}

int mouse_read(int fd, mouse_evt_t* evt) {
	if(core_get_active_ux() != core_get_ux())
        return 0;

    int8_t mv[MOUSE_EVT_MAX];
    if(read(fd, mv, MOUSE_EVT_MAX) == MOUSE_EVT_MAX) {
        if(mv[0] != 0)  {
            mouse_evt(mv[1], mv[2], mv[3], evt);
			return 1;
		}
    }
	return 0;
}

#ifdef __cplusplus
}
#endif