#ifndef X_H
#define X_H

#include <graph/graph_ex.h>
#include <x/xcntl.h>
#include <x/xevent.h>
#include <x/xtheme.h>

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
void     x_get_work_dir(char* ret, uint32_t len);
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
