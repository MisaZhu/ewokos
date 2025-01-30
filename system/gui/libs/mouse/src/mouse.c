#include <mouse/mouse.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define MOUSE_TIMER 3000

static int8_t _mouse_down = 0;

static void input(int8_t bt, int8_t rx, int8_t ry, 
        mouse_evt_handle_t handle,
        void* p) {
	uint8_t state = MOUSE_STATE_MOVE;
	uint8_t button = MOUSE_BUTTON_NONE;

	if(bt == 2) 
		button = MOUSE_BUTTON_LEFT;
	else if(bt == 3) 
		button = MOUSE_BUTTON_MID;
	else if(bt == 4) 
		button = MOUSE_BUTTON_RIGHT;
	else if(bt == 8) {
		if(rx > 0)
			button = MOUSE_BUTTON_SCROLL_UP;
		else
			button = MOUSE_BUTTON_SCROLL_DOWN;
		rx = 0;
		ry = 0;
	}

	if((bt > 1 && bt != 8)||
			(bt == 0 && _mouse_down == 1)) {//down
		state = MOUSE_STATE_DOWN;
		_mouse_down = 1;
	}
	else if(bt == 1) {//up
		state = MOUSE_STATE_UP;
		_mouse_down = 0;
	}

    handle(state, button, rx, ry, p);
}

void mouse_read(int fd, mouse_evt_handle_t handle, void* p) {
    int8_t mv[4];
    if(read(fd, mv, 4) == 4) {
        if(mv[0] != 0) 
            input(mv[1], mv[2], mv[3], handle, p);
    }
    usleep(MOUSE_TIMER);
}

#ifdef __cplusplus
}
#endif