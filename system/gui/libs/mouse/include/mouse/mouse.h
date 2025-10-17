#ifndef MOUSE_H
#define MOUSE_H 

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	MOUSE_TYPE_REL = 1,
	MOUSE_TYPE_ABS = 2
};

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
	uint8_t type;
	uint8_t state;
	uint8_t button;
	uint8_t scroll;
	int16_t x;
	int16_t y;
} __attribute__((packed)) mouse_evt_t;

int mouse_read(int keyb_fd, mouse_evt_t* evt);

#ifdef __cplusplus
}
#endif

#endif
