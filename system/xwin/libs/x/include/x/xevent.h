#ifndef XEVENT_H
#define XEVENT_H

#include <ewoksys/ewokdef.h>
#include <mouse/mouse.h>

#ifdef __cplusplus
extern "C" {
#endif

#define X_EVT_BLOCK_EVT			128

enum {
	XEVT_NONE = 0,
	XEVT_MOUSE,
	XEVT_IM, //input method
	XEVT_WIN
};

enum {
	XEVT_WIN_CLOSE = 0,
	XEVT_WIN_MOVE,
	XEVT_WIN_MOVE_TO,
	XEVT_WIN_MAX,
	XEVT_WIN_RESIZE,
	XEVT_WIN_FOCUS,
	XEVT_WIN_UNFOCUS,
	XEVT_WIN_VISIBLE,
	XEVT_WIN_REPAINT,
	XEVT_WIN_REORG,
};

enum {
	XIM_STATE_PRESS = 0,
	XIM_STATE_RELEASE
};

typedef struct {
	uint8_t type;
	int32_t state;
	uint32_t win;
	union {
		struct {
			int32_t relative;
			int32_t x, y;
			int32_t rx, ry;
			int32_t from_x, from_y; //drag from(first press down position)
			int32_t button;
		} mouse;

		struct {
			int32_t value;
		} im;

		struct {
			int32_t event;
			int32_t v0;
			int32_t v1;
		} window;
	} value;
} xevent_t;

#ifdef __cplusplus
}
#endif

#endif
