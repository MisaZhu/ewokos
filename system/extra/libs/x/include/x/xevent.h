#ifndef XEVENT_H
#define XEVENT_H

#include <sys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif


enum {
	XEVT_NONE = 0,
	XEVT_MOUSE,
	XEVT_IM, //input method
	XEVT_WIN
};

enum {
	XEVT_MOUSE_MOVE = 0,
	XEVT_MOUSE_DRAG,
	XEVT_MOUSE_DOWN,
	XEVT_MOUSE_UP
};

enum {
	XEVT_WIN_CLOSE = 0,
	XEVT_WIN_MOVE,
	XEVT_WIN_MOVE_TO,
	XEVT_WIN_MAX,
	XEVT_WIN_RESIZE,
	XEVT_WIN_FOCUS,
	XEVT_WIN_UNFOCUS,
	XEVT_WIN_VISIBLE
};

typedef struct {
	uint8_t type;
	int32_t state;
	uint32_t win;
	union {
		struct {
			int32_t x, y;
			int32_t winx, winy;
			int32_t rx, ry;
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
