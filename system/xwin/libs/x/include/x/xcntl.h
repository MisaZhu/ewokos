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
	X_DCNTL_SET_XWM_THEME,
	X_DCNTL_SHOW_CURSOR,
	X_DCNTL_NEXT_FOCUS,
	X_DCNTL_CLOSE_FOCUS,
	X_DCNTL_LAUNCHER,
	X_DCNTL_GET_EVT_NODE
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
#define XWIN_STYLE_MAX            0x800

#define XWIN_STYLE_SPRITE         (XWIN_STYLE_NO_FRAME | XWIN_STYLE_NO_BG_EFFECT)

#define XWIN_TITLE_MAX 32
#define X_APP_NAME_MAX 128
typedef struct {
	ewokos_addr_t win;
	int32_t  ws_g_shm_id;
	int32_t  frame_g_shm_id;
	uint32_t display_index;
	uint32_t style;
	uint32_t state;

	bool update_theme;
	bool visible;
	bool is_main;
	bool alpha;
	bool focused;
	bool hide_cursor;
	bool ws_g_shm_contig; /* ws_g shm backing is physically contiguous
	                           (IPC_CONTIG); published by the server together
	                           with ws_g_shm_id so clients can mark their
	                           workspace graph g2d-capable */
	bool frame_g_shm_contig; /* same as ws_g_shm_contig, for the frame
	                           graph published via frame_g_shm_id (xwm draws
	                           window frames through g2d on it) */
	grect_t wsr; //workspace rect
	grect_t winr; //window rect
	char title[XWIN_TITLE_MAX];
	char name[X_APP_NAME_MAX];

	/*shm-based UPDATE handshake (replaces the XWIN_CNTL_UPDATE IPC fast
	  path). Client paints into ws_g, publishes its calling thread pid into
	  update_pid, issues a full memory barrier, then sets update_requested=1
	  and blocks on token=xinfo->win. Server polls the flag from loop_step at
	  its own fps, snapshots ws_g into ws_g_buffer, clears the flag and wakes
	  update_pid. update_pid must be the CURRENT THREAD pid (thread_get_id,
	  not getpid: getpid returns the root task pid, and proc_wakeup_by
	  targets one specific proc entry, so waking the wrong thread would
	  leave the blocked painter stuck). Both fields are volatile and the
	  barrier pairs with __sync_synchronize on ARM (dmb ish) so the server
	  never sees update_requested=1 with a stale ws_g.*/
	volatile uint32_t update_requested;
	volatile int32_t  update_pid;

	/*fps_async double-buffering (x.json "fps_async":1). The server allocates a
	  second workspace buffer ws_g2 (graph_new_shm) and publishes its shm id here.
	  The client render target is ALWAYS ws_g (never alternated): framebuffer-style
	  clients such as the SDL2 ewokos backend cache the pixel pointer handed back at
	  window-create time and blit every frame into that one fixed buffer, so the
	  render target must stay stable. The "flip" is therefore an explicit copy the
	  client does in xwin_repaint: only when the server has consumed the previous
	  submission (update_requested==0) does it blit ws_g -> ws_g2, set front_index=1
	  and update_requested=1 - all without blocking, so the client keeps its own fps.
	  The server snapshots ws_g[front_index] (= ws_g2) into ws_g_buffer at its own
	  fps and clears update_requested only AFTER that copy, so ws_g2 is stable while
	  the server reads it and the client can keep painting ws_g without tearing.
	  front_index selects the server's snapshot source (1 => ws_g2); back_index is
	  reserved and stays 0 (the client always renders ws_g).*/
	bool fps_async;
	int32_t ws_g2_shm_id;
	bool ws_g2_shm_contig;
	volatile uint32_t back_index;
	volatile uint32_t front_index;

	/*the server's private composite source (ws_g_buffer) shm, published so xwm
	  blends decorations over the SAME stable snapshot the compositor reads,
	  never the client's live render buffer. In fps_async mode the client may be
	  painting into ws_g or ws_g2 at any moment, so xwm reading those directly
	  would sample a half-drawn frame; ws_g_buffer is server-owned and the client
	  never touches it, so it is always consistent with frame_g (prepare_win_content
	  blits frame_g from it). Both modes use this, so xwm behaviour is uniform.*/
	int32_t ws_g_buffer_shm_id;
	bool ws_g_buffer_shm_contig;
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
