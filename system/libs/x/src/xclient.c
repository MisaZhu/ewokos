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

int x_update_info(x_t* x, xinfo_t* info) {
	proto_t in;
	PF->init(&in, NULL, 0)->add(&in, info, sizeof(xinfo_t));
	int ret = fcntl_raw(x->fd, X_CNTL_UPDATE_INFO, &in, NULL);
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

x_t* x_open(int x, int y, int w, int h, const char* title, int style) {
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

	x_t* ret = (x_t*)malloc(sizeof(x_t));
	memset(ret, 0, sizeof(x_t));

	ret->fd = fd;
	ret->closed = false;

	xinfo_t xinfo;
	xinfo.style = style;
	xinfo.state = X_STATE_NORMAL;
	memcpy(&xinfo.wsr, &r, sizeof(grect_t));
	strncpy(xinfo.title, title, X_TITLE_MAX-1);
	x_update_info(ret, &xinfo);
	return ret;
}

int x_get_info(x_t* x, xinfo_t* info) {
	if(x == NULL || info == NULL)
		return -1;
	
	proto_t out;
	PF->init(&out, NULL, 0);
	if(fcntl_raw(x->fd, X_CNTL_GET_INFO, NULL, &out) != 0)
		return -1;
	proto_read_to(&out, info, sizeof(xinfo_t));
	PF->clear(&out);
	return 0;
}

static graph_t* x_get_graph(x_t* x) {
	if(x == NULL)
		return NULL;

	xinfo_t info;
	if(x_get_info(x, &info) != 0)
		return NULL;
	void* gbuf = shm_map(info.shm_id);
	if(gbuf == NULL)
		return NULL;
	return graph_new(gbuf, info.wsr.w, info.wsr.h);
}

static void x_release_graph(x_t* x, graph_t* g) {
	xinfo_t info;
	if(x_get_info(x, &info) != 0)
		return;
	graph_free(g);
	shm_unmap(info.shm_id);
}

void x_close(x_t* x) {
	if(x == NULL)
		return;
	close(x->fd);
	free(x);
}

static int win_event_handle(x_t* x, xevent_t* ev) {
	if(ev->value.window.event == XEVT_WIN_MOVE) {
	}
	else if(ev->value.window.event == XEVT_WIN_CLOSE) {
		x->closed = true;
	}
	else if(ev->value.window.event == XEVT_WIN_FOCUS) {
		if(x->on_focus) {
			x->on_focus(x);
			x_repaint(x);
		}
	}
	else if(ev->value.window.event == XEVT_WIN_UNFOCUS) {
		if(x->on_unfocus) {
			x->on_unfocus(x);
			x_repaint(x);
		}
	}
	else if(ev->value.window.event == XEVT_WIN_MAX) {
		xinfo_t xinfo;
		x_get_info(x, &xinfo);
		if(xinfo.state == X_STATE_MAX) {
			memcpy(&xinfo.wsr, &x->xinfo_prev.wsr, sizeof(grect_t));
			xinfo.state = x->xinfo_prev.state;
		}
		else {
			xscreen_t scr;
			if(x_screen_info(&scr) == 0) {
				memcpy(&x->xinfo_prev, &xinfo, sizeof(xinfo_t));
				grect_t r = {0, 0, scr.size.w, scr.size.h};
				x_get_workspace(x->fd, xinfo.style, &r, &r);
				memcpy(&xinfo.wsr, &r, sizeof(grect_t));
				xinfo.state = X_STATE_MAX;
			}
		}
		x_update_info(x, &xinfo);
		if(x->on_resize) {
			x->on_resize(x);
			x_repaint(x);
		}
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

int x_is_top(x_t* x) {
	proto_t out;
	PF->init(&out, NULL, 0);

	int res = -1;
	if(fcntl_raw(x->fd, X_CNTL_IS_TOP, NULL, &out) == 0) {
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

int x_set_visible(x_t* x, bool visible) {
	proto_t in;
	PF->init(&in, NULL, 0)->addi(&in, visible);

	int res = fcntl_raw(x->fd, X_CNTL_SET_VISIBLE, &in, NULL);
	PF->clear(&in);
	if(x->on_focus)
		x->on_focus(x);
	x_repaint(x);
	return res;
}

void x_repaint(x_t* x) {
	if(x->on_repaint == NULL) {
		fcntl_raw(x->fd, X_CNTL_UPDATE, NULL, NULL);
		return;
	}

	graph_t* g = x_get_graph(x);
	x->on_repaint(x, g);
	fcntl_raw(x->fd, X_CNTL_UPDATE, NULL, NULL);
	x_release_graph(x, g);
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

void  x_run(x_t* x) {
	if(x->on_init != NULL)
		x->on_init(x);
	
	int ipc_pid = ipc_serv_run(handle, x, true);

	xevent_t xev;
	while(!x->closed) {
		if(x_get_event(x, &xev) == 0) {
			if(xev.type == XEVT_WIN) 
				win_event_handle(x, &xev);

			if(x->on_event != NULL)
				x->on_event(x, &xev);
		}
		else {
			if(x->on_loop == NULL) {
				proc_block(ipc_pid, (int32_t)x);
			}
			else {
				x->on_loop(x);
				usleep(10000);
			}
		}
	}
}
