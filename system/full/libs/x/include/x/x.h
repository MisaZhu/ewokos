#ifndef X_H
#define X_H

#include <graph/graph.h>
#include <fonts/fonts.h>
#include <x/xcntl.h>
#include <x/xevent.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct st_xevent {
  xevent_t event;
  struct st_xevent* next;
} x_event_t;

struct st_xwin;

typedef struct st_x {
	struct st_xwin* main_win;
	void* data;
	bool terminated;
	x_event_t* event_head;
	x_event_t* event_tail;

	void (*on_loop)(void* p);
} x_t;

void     x_push_event(x_t* x, xevent_t* ev);
int      x_screen_info(xscreen_t* scr, uint32_t index);
int      x_get_display_num(void);
void     x_init(x_t* x, void* data);
void     x_run(x_t* x, void* loop_data);

#ifdef __cplusplus
}
#endif

#endif
