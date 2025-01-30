#ifndef MOUSE_H
#define MOUSE_H 

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void(*mouse_evt_handle_t)(uint8_t state,
        uint8_t button,
        int32_t rx,
        int32_t ry,
		void* p);

enum {
	MOUSE_STATE_MOVE = 1,
	MOUSE_STATE_DRAG,
	MOUSE_STATE_DOWN,
	MOUSE_STATE_UP,
	MOUSE_STATE_CLICK
};

enum {
	MOUSE_BUTTON_NONE = 0,
	MOUSE_BUTTON_LEFT,
	MOUSE_BUTTON_MID,
	MOUSE_BUTTON_RIGHT,
	MOUSE_BUTTON_SCROLL_UP,
	MOUSE_BUTTON_SCROLL_DOWN
};

void mouse_read(int keyb_fd, mouse_evt_handle_t handle_func, void* p);

#ifdef __cplusplus
}
#endif

#endif