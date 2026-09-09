#ifndef X_H
#define X_H

#include <graph/graph_ex.h>
#include <x/xcntl.h>
#include <x/xevent.h>
#include <x/xtheme.h>
#include <ewoksys/vfs.h>

#ifdef __cplusplus
extern "C" {
#endif

#define X_SYSTEM_PATH       "/usr/system"

typedef struct st_xevent {
  xevent_t event;
  struct st_xevent* next;
} x_event_t;

struct st_xwin;

typedef struct st_x {
	struct st_xwin* main_win;
	struct st_xwin* prompt_win;
	void* data;
	bool terminated;
	x_event_t* event_head;
	x_event_t* event_tail;

	void (*on_loop)(void* p);
	fsinfo_t dev_fsinfo;
	uint32_t evt_node;
} x_t;

int      x_exec(const char* fname);
int      x_set_top_app(const char* fname);
int      x_set_app_name(x_t* x, const char* fname);
int      x_show_cursor(bool show);
void     x_push_event(x_t* x, xevent_t* ev);
uint32_t x_get_display_id(int32_t index_def);
int      x_screen_info(xscreen_info_t* scr, int32_t index);
int      x_fetch_screen_graph(uint32_t index, graph_t* g);
int      x_get_display_num(void);
void     x_init(x_t* x, void* data);
int      x_run(x_t* x, void* loop_data);
void     x_terminate(x_t* x);

/*
 * The two halves of x_run()'s loop body, published for event loops that are not
 * ours to own.  An app with no foreign toolkit calls x_run() and never sees
 * these; a toolkit that brings its own loop - a Qt QPA platform plugin is the
 * case that motivated them - has to interleave the window server's events with
 * its own timers, posted events and socket notifiers, and cannot do that from
 * inside x_run()'s while().  Before these existed the only choices were to run
 * x_run() on a second thread and marshal events across, or to duplicate the
 * X_DCNTL_GET_EVT round trip and drift from it.
 *
 * x_run() itself now calls both, so there is one copy of the routing.
 *
 *   x_poll_event   non-blocking: drains x_push_event()'s local queue first, then
 *                  asks the server once.  Returns 0 and fills *ev on success, -1
 *                  when there is nothing to take.  It never parks - the blocking
 *                  path inside libx waits on x->evt_node with vfs_block(), which
 *                  takes no timeout, so a loop that also drives timers would
 *                  stall for as long as the server stayed quiet.
 *   xwin_dispatch_event   routes one event the way x_run() does, including the
 *                  registry lookup that turns ev->win back into an xwin_t (menus,
 *                  submenus and dialogs are windows beyond main/prompt), the
 *                  XEVT_WIN split to xwin_event_handle(), and the prompt_win
 *                  guard on on_event.  Takes the x_t for symmetry with the rest
 *                  of this API; the event already carries the window handle.
 *
 * Neither touches xwin_retry_pending_presents() or x->terminated, both of which
 * stay the caller's business: a foreign loop decides its own exit condition and
 * its own present cadence.
 */
int      x_poll_event(x_t* x, xevent_t* ev);
void     xwin_dispatch_event(x_t* x, xevent_t* ev);
const char*  x_get_own_dir(char* ret, uint32_t len);
int      x_get_theme(x_theme_t* theme);
const char* x_get_theme_fname(const char* prefix,
			const char* app_name,
			const char* fname,
			char* ret,
			uint32_t len);
int      x_get_desktop_space(int disp_index, grect_t* r);
int      x_set_desktop_space(int disp_index, const grect_t* r);
const char* x_get_res_name(const char* name, char* ret, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif
