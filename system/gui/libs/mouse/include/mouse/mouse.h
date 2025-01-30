#ifndef MOUSE_H
#define MOUSE_H 

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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

typedef struct {
	int8_t state;
	int8_t button;
	int32_t rx;
	int32_t ry;
} mouse_evt_t;

int mouse_read(int keyb_fd, mouse_evt_t* evt);

#ifdef __cplusplus
}
#endif

#endif