#include <x/xclient.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/syscall.h>
#include <sys/proc.h>
#include <sys/vdevice.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

int x_update_info(xwin_t* xwin, const xinfo_t* info) {
	proto_t in;
	PF->init(&in, NULL, 0)->add(&in, info, sizeof(xinfo_t));
	int ret = fcntl_raw(xwin->fd, X_CNTL_UPDATE_INFO, &in, NULL);
	PF->clear(&in);
	return ret;
}

static int  x_get_workspace(int xfd, int style, grect_t* frame, grect_t* workspace) {
	proto_t in, out;
	PF->init(&out, NULL, 0);

	PF->init(&in, NULL, 0)->addi(&in, style)->add(&in, frame, sizeof(grect_t));
	int ret = fcntl_raw(xfd, X_CNTL_WORKSPACE, &in, &out);
	PF->clear(&in);
	if(ret == 0) 
		proto_read_to(&out, workspace, sizeof(grect_t));
	PF->clear(&out);
	return ret;
}

xwin_t* x_open(x_t* xp, int x, int y, int w, int h, const char* title, int style) {
	if(w <= 0 || h <= 0)
		return NULL;

	int fd = open("/dev/x", O_RDWR);
	if(fd < 0)
		return NULL;

	grect_t r;
	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;
	x_get_workspace(fd, style, &r, &r);

	xwin_t* ret = (xwin_t*)malloc(sizeof(xwin_t));
	memset(ret, 0, sizeof(xwin_t));
	ret->fd = fd;

	ret->x = xp;
	if(xp->main_win == NULL)
		xp->main_win = ret;

	xinfo_t xinfo;
	xinfo.win = (uint32_t)ret;
	xinfo.style = style;
	xinfo.state = X_STATE_NORMAL;
	memcpy(&xinfo.wsr, &r, sizeof(grect_t));
	strncpy(xinfo.title, title, X_TITLE_MAX-1);
	x_update_info(ret, &xinfo);
	return ret;
}

int x_get_info(xwin_t* xwin, xinfo_t* info) {
	if(xwin == NULL || info == NULL)
		return -1;
	
	proto_t out;
	PF->init(&out, NULL, 0);
	if(fcntl_raw(xwin->fd, X_CNTL_GET_INFO, NULL, &out) != 0)
		return -1;
	proto_read_to(&out, info, sizeof(xinfo_t));
	PF->clear(&out);
	return 0;
}

static graph_t* x_get_graph(xwin_t* xwin) {
	if(xwin == NULL)
		return NULL;

	xinfo_t info;
	if(x_get_info(xwin, &info) != 0)
		return NULL;
	void* gbuf = shm_map(info.shm_id);
	if(gbuf == NULL)
		return NULL;
	return graph_new(gbuf, info.wsr.w, info.wsr.h);
}

static void x_release_graph(xwin_t* xwin, graph_t* g) {
	xinfo_t info;
	if(x_get_info(xwin, &info) != 0)
		return;
	graph_free(g);
	shm_unmap(info.shm_id);
}

void x_close(xwin_t* xwin) {
	if(xwin == NULL)
		return;
	if(xwin->on_close) {
		xwin->on_close(xwin);
	}

	close(xwin->fd);

	if(xwin->x->main_win == xwin)
		xwin->x->terminated = true;
	free(xwin);
}

static int win_event_handle(xwin_t* xwin, xevent_t* ev) {
	if(ev->value.window.event == XEVT_WIN_CLOSE) {
		if(xwin->x->main_win == xwin)
			xwin->x->terminated = true;
	}
	else if(ev->value.window.event == XEVT_WIN_FOCUS) {
		if(xwin->on_focus) {
			xwin->on_focus(xwin);
			x_repaint(xwin);
		}
	}
	else if(ev->value.window.event == XEVT_WIN_UNFOCUS) {
		if(xwin->on_unfocus) {
			xwin->on_unfocus(xwin);
			x_repaint(xwin);
		}
	}
	else if(ev->value.window.event == XEVT_WIN_RESIZE) {
		xinfo_t xinfo;
		x_get_info(xwin, &xinfo);
		xinfo.wsr.w += ev->value.window.v0;
		xinfo.wsr.h += ev->value.window.v1;
		x_update_info(xwin, &xinfo);
		if(xwin->on_resize) {
			xwin->on_resize(xwin);
		}
		x_repaint(xwin);
	}
	else if(ev->value.window.event == XEVT_WIN_MOVE) {
		xinfo_t xinfo;
		x_get_info(xwin, &xinfo);
		xinfo.wsr.x += ev->value.window.v0;
		xinfo.wsr.y += ev->value.window.v1;
		x_update_info(xwin, &xinfo);
	}
	else if(ev->value.window.event == XEVT_WIN_MAX) {
		xinfo_t xinfo;
		x_get_info(xwin, &xinfo);
		if(xinfo.state == X_STATE_MAX) {
			memcpy(&xinfo.wsr, &xwin->xinfo_prev.wsr, sizeof(grect_t));
			xinfo.state = xwin->xinfo_prev.state;
		}
		else {
			xscreen_t scr;
			if(x_screen_info(&scr) == 0) {
				memcpy(&xwin->xinfo_prev, &xinfo, sizeof(xinfo_t));
				grect_t r = {0, 0, scr.size.w, scr.size.h};
				x_get_workspace(xwin->fd, xinfo.style, &r, &r);
				memcpy(&xinfo.wsr, &r, sizeof(grect_t));
				xinfo.state = X_STATE_MAX;
			}
		}
		x_update_info(xwin, &xinfo);
		if(xwin->on_resize) {
			xwin->on_resize(xwin);
		}
		x_repaint(xwin);
	}
	return 0;
}

static void x_push_event(x_t* x, xevent_t* ev) {
	x_event_t* e = (x_event_t*)malloc(sizeof(x_event_t));
	e->next = NULL;
	memcpy(&e->event, ev, sizeof(xevent_t));

  if(x->event_tail != NULL)
    x->event_tail->next = e;
  else
    x->event_head = e;
  x->event_tail = e;
}

static int x_get_event(x_t* x, xevent_t* ev) {
  x_event_t* e = x->event_head;
	if(e == NULL)
		return -1;

  x->event_head = x->event_head->next;
  if(x->event_head == NULL)
    x->event_tail = NULL;

  memcpy(ev, &e->event, sizeof(xevent_t));
  free(e);
  return 0;
}

int x_is_top(xwin_t* xwin) {
	proto_t out;
	PF->init(&out, NULL, 0);

	int res = -1;
	if(fcntl_raw(xwin->fd, X_CNTL_IS_TOP, NULL, &out) == 0) {
		res = proto_read_int(&out);
	}
	PF->clear(&out);
	return res;
}

int x_screen_info(xscreen_t* scr) {
	proto_t out;
	PF->init(&out, NULL, 0);

	int ret = dev_cntl("/dev/x", X_DCNTL_GET_INFO, NULL, &out);
	if(ret == 0)
		proto_read_to(&out, scr, sizeof(xscreen_t));
	PF->clear(&out);
	return ret;
}

int x_set_visible(xwin_t* xwin, bool visible) {
	proto_t in;
	PF->init(&in, NULL, 0)->addi(&in, visible);

	int res = fcntl_raw(xwin->fd, X_CNTL_SET_VISIBLE, &in, NULL);
	PF->clear(&in);
	if(xwin->on_focus)
		xwin->on_focus(xwin);
	x_repaint(xwin);
	return res;
}

void x_repaint(xwin_t* xwin) {
	if(xwin->on_repaint == NULL) {
		fcntl_raw(xwin->fd, X_CNTL_UPDATE, NULL, NULL);
		return;
	}

	graph_t* g = x_get_graph(xwin);
	xwin->on_repaint(xwin, g);
	fcntl_raw(xwin->fd, X_CNTL_UPDATE, NULL, NULL);
	x_release_graph(xwin, g);
}

static void handle(int from_pid, int cmd, proto_t* in, proto_t* out, void* p) {
	(void)from_pid;
	(void)out;
	x_t* x = (x_t*)p;

	if(cmd == X_CMD_PUSH_EVENT) {
		xevent_t ev;
		proto_read_to(in, &ev, sizeof(xevent_t));
		x_push_event(x, &ev);
	}

	if(x->on_loop == NULL)
		proc_wakeup((int32_t)x);
}

void  x_init(x_t* x, void* data) {
	memset(x, 0, sizeof(x_t));
	x->data = data;
}

void  x_run(x_t* x, void* loop_data) {
	int ipc_pid = ipc_serv_run(handle, x, true);

	xevent_t xev;
	while(!x->terminated) {
		if(x_get_event(x, &xev) == 0) {
			xwin_t* xwin = (xwin_t*)xev.win;
			if(xwin != NULL) {

				if(xev.type == XEVT_WIN) {
					win_event_handle(xwin, &xev);
				}

				if(xwin->on_event != NULL)
					xwin->on_event(xwin, &xev);
			}
		}
		else {
			if(x->on_loop == NULL) {
				proc_block(ipc_pid, (int32_t)x);
			}
			else {
				x->on_loop(loop_data);
				usleep(10000);
			}
		}
	}
}
