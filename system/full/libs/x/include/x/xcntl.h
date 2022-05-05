#ifndef XCNTL_H
#define XCNTL_H

#include <sys/ewokdef.h>
#include <graph/graph.h>

#ifdef __cplusplus
extern "C" {
#endif


enum {
	X_CNTL_NONE = 0,
	X_CNTL_NEW,
	X_CNTL_UPDATE,
	X_CNTL_UPDATE_INFO,
	X_CNTL_GET_INFO,
	X_CNTL_WORKSPACE,
	X_CNTL_SET_VISIBLE,
	X_CNTL_CALL_XIM
};

enum {
	X_DCNTL_GET_INFO = 0,
	X_DCNTL_SET_XWM,
	X_DCNTL_UNSET_XWM,
	X_DCNTL_INPUT
};

enum {
	X_STATE_NORMAL = 0,
	X_STATE_MAX,
	X_STATE_MIN,
	X_STATE_FULL_SCREEN
};

enum {
	X_CMD_PUSH_EVENT = 0
};

#define X_STYLE_NORMAL    0x0
#define X_STYLE_NO_FRAME  0x1
#define X_STYLE_NO_TITLE  0x2
#define X_STYLE_NO_RESIZE 0x4
#define X_STYLE_ALPHA     0x8
#define X_STYLE_NO_FOCUS  0x10
#define X_STYLE_SYSTOP    0x20
#define X_STYLE_SYSBOTTOM 0x40
#define X_STYLE_XIM       0x80

#define X_TITLE_MAX 64
typedef struct {
	uint32_t win;
	int shm_id;
	int style;
	int state;
	bool visible;
	grect_t wsr; //workspace rect
	char title[X_TITLE_MAX];
} xinfo_t;

typedef struct {
	int id;
	int fps;
	gsize_t size;
} xscreen_t;

#ifdef __cplusplus
}
#endif

#endif
