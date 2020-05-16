#include <x/xclient.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

int x_update(x_t* x) {
	int ret = fcntl_raw(x->fd, X_CNTL_UPDATE, NULL, NULL);
	return ret;
}

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
	memcpy(&xinfo.r, &r, sizeof(grect_t));
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

graph_t* x_get_graph(x_t* x) {
	if(x == NULL)
		return NULL;

	xinfo_t info;
	if(x_get_info(x, &info) != 0)
		return NULL;
	void* gbuf = shm_map(info.shm_id);
	if(gbuf == NULL)
		return NULL;
	return graph_new(gbuf, info.r.w, info.r.h);
}

void  x_release_graph(x_t* x, graph_t* g) {
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

static int win_event_handle(x_t* x, xevent_t* ev, void* p) {
	if(ev->value.window.event == XEVT_WIN_MOVE) {
	}
	else if(ev->value.window.event == XEVT_WIN_CLOSE) {
		x->closed = true;
	}
	else if(ev->value.window.event == XEVT_WIN_FOCUS) {
		if(x->on_focus) 
			x->on_focus(x, p);
	}
	else if(ev->value.window.event == XEVT_WIN_UNFOCUS) {
		if(x->on_unfocus) 
			x->on_unfocus(x, p);
	}
	else if(ev->value.window.event == XEVT_WIN_MAX) {
		xinfo_t xinfo;
		x_get_info(x, &xinfo);
		if(xinfo.state == X_STATE_MAX) {
			memcpy(&xinfo.r, &x->xinfo_prev.r, sizeof(grect_t));
			xinfo.state = x->xinfo_prev.state;
		}
		else {
			xscreen_t scr;
			if(x_screen_info(&scr) == 0) {
				memcpy(&x->xinfo_prev, &xinfo, sizeof(xinfo_t));
				grect_t r = {0, 0, scr.size.w, scr.size.h};
				x_get_workspace(x->fd, xinfo.style, &r, &r);
				memcpy(&xinfo.r, &r, sizeof(grect_t));
				xinfo.state = X_STATE_MAX;
			}
		}
		x_update_info(x, &xinfo);
		if(x->on_resize)
			x->on_resize(x, p);
	}
	return 0;
}

int x_get_event_raw(x_t* x, xevent_t* ev) {
	proto_t out;
	PF->init(&out, NULL, 0);

	int res = -1;
	if(fcntl_raw(x->fd, X_CNTL_GET_EVT, NULL, &out) == 0) {
		proto_read_to(&out, ev, sizeof(xevent_t));
		res = 0;
	}
	PF->clear(&out);
	return res;
}

int x_get_event(x_t* x, xevent_t* ev, void* p) {
	if(x_get_event_raw(x, ev) == 0) {
		if(ev->type == XEVT_WIN) 
			win_event_handle(x, ev, p);
		return 0;
	}
	return -1;
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

	int ret = finfo("/dev/x", &out);
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
	return res;
}

void  x_run(x_t* x, x_handle_func_t event_handle_func, x_step_func_t step_func, void* p) {
	xevent_t xev;
	while(!x->closed) {
		if(x_get_event_raw(x, &xev) == 0) {
			if(xev.type == XEVT_WIN) 
				win_event_handle(x, &xev, p);

			if(event_handle_func != NULL)
				event_handle_func(x, &xev, p);
		}
		if(step_func != NULL)
			step_func(x, p);
		usleep(10000);
	}
}
