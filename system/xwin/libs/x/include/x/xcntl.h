#ifndef XCNTL_H
#define XCNTL_H

#include <ewoksys/ewokdef.h>
#include <graph/graph.h>

#ifdef __cplusplus
extern "C" {
#endif


enum {
	XWIN_CNTL_NONE = 0,
	XWIN_CNTL_NEW,
	XWIN_CNTL_UPDATE,
	XWIN_CNTL_UPDATE_INFO,
	XWIN_CNTL_WORK_SPACE,
	XWIN_CNTL_CALL_XIM,
	XWIN_CNTL_TOP,
	XWIN_CNTL_TRY_FOCUS,
	XWIN_CNTL_SET_BUSY
};

enum {
	X_DCNTL_GET_INFO = 0,
	X_DCNTL_GET_DISP_NUM,
	X_DCNTL_SET_XWM,
	X_DCNTL_UNSET_XWM,
	X_DCNTL_INPUT,
	X_DCNTL_GET_EVT,
	X_DCNTL_QUIT,
	X_DCNTL_SET_TOP,
	X_DCNTL_GET_DESKTOP_SPACE,
	X_DCNTL_SET_DESKTOP_SPACE,
	X_DCNTL_LOAD_THEME,
	X_DCNTL_GET_THEME,
	X_DCNTL_SET_THEME,
	X_DCNTL_LOAD_XWM_THEME,
	X_DCNTL_GET_XWM_THEME,
	X_DCNTL_SET_XWM_THEME
};

enum {
	XWIN_STATE_NORMAL = 0,
	XWIN_STATE_MAX,
	XWIN_STATE_MIN,
	XWIN_STATE_FULL_SCREEN
};

#define X_UPDATE_REBUILD 0x1
#define X_UPDATE_REFRESH 0x2

#define XWIN_STYLE_NORMAL         0x0
#define XWIN_STYLE_NO_FRAME       0x1
#define XWIN_STYLE_NO_TITLE       0x2
#define XWIN_STYLE_NO_RESIZE      0x4
#define XWIN_STYLE_LAZY           0x8 //ignore all event
#define XWIN_STYLE_NO_FOCUS       0x10
#define XWIN_STYLE_SYSTOP         0x20
#define XWIN_STYLE_SYSBOTTOM      0x40
#define XWIN_STYLE_XIM            0x80
#define XWIN_STYLE_LAUNCHER       0x100
#define XWIN_STYLE_PROMPT         0x200 //prompt win
#define XWIN_STYLE_NO_BG_EFFECT   0x400

#define XWIN_TITLE_MAX 32
#define X_APP_NAME_MAX 128
typedef struct {
	uint32_t win;
	int32_t  ws_g_shm_id;
	int32_t  frame_g_shm_id;
	uint32_t display_index;
	uint32_t style;
	uint32_t state;
	bool visible;
	bool is_main;
	bool covered;
	bool alpha;
	bool focused;
	grect_t wsr; //workspace rect
	grect_t winr; //window rect
	char title[XWIN_TITLE_MAX];
	char name[X_APP_NAME_MAX];
} xinfo_t;

typedef struct {
	int32_t id;
	uint32_t fps;
	int32_t g_shm_id;
	gsize_t size;
} xscreen_info_t;

#ifdef __cplusplus
}
#endif

#endif
