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
	proto_init(&in, NULL, 0);
	proto_add(&in, info, sizeof(xinfo_t));
	int ret = fcntl_raw(x->fd, X_CNTL_UPDATE_INFO, &in, NULL);
	proto_clear(&in);
	return ret;
}

static int  x_get_workspace(int xfd, int style, grect_t* frame, grect_t* workspace) {
	proto_t in, out;
	proto_init(&in, NULL, 0);
	proto_init(&out, NULL, 0);

	proto_add_int(&in, style);
	proto_add(&in, frame, sizeof(grect_t));
	int ret = fcntl_raw(xfd, X_CNTL_WORKSPACE, &in, &out);
	proto_clear(&in);
	if(ret == 0) 
		proto_read_to(&out, workspace, sizeof(grect_t));
	proto_clear(&out);
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
	ret->closed = 0;

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
	proto_init(&out, NULL, 0);
	if(fcntl_raw(x->fd, X_CNTL_GET_INFO, NULL, &out) != 0)
		return -1;
	proto_read_to(&out, info, sizeof(xinfo_t));
	proto_clear(&out);
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

static int win_event_handle(x_t* x, xevent_t* ev) {
	if(ev->value.window.event == XEVT_WIN_MOVE) {
	}
	else if(ev->value.window.event == XEVT_WIN_CLOSE) {
		x->closed = 1;
	}
	else if(ev->value.window.event == XEVT_WIN_FOCUS) {
		if(x->on_focus) 
			x->on_focus(x, x->data);
	}
	else if(ev->value.window.event == XEVT_WIN_UNFOCUS) {
		if(x->on_unfocus) 
			x->on_unfocus(x, x->data);
	}
	else if(ev->value.window.event == XEVT_WIN_MAX) {
		/*
		if(x->xinfo.state == X_STATE_MAX) {
			x_resize_to(x, x->xinfo_prev.r.x, 
					x->xinfo_prev.r.y,
					x->xinfo_prev.r.w,
					x->xinfo_prev.r.h);
			if(x->on_restore != NULL)
				x->on_restore(x, x->data);
			x->xinfo.state = x->xinfo_prev.state;
		}
		else {
			xscreen_t scr;
			if(x_screen_info(&scr) == 0) {
				memcpy(&x->xinfo_prev, &x->xinfo, sizeof(xinfo_t));
				grect_t r;
				r.x = 0;
				r.y = 0;
				r.w = scr.size.w;
				r.h = scr.size.h;
				x_get_workspace(x->fd, x->xinfo.style, &r, &r);
				x_resize_to(x, r.x, r.y, r.w, r.h);
				if(x->on_max != NULL)
					x->on_max(x, x->data);
				x->xinfo.state = X_STATE_MAX;
			}
		}
		*/
	}
	return 0;
}

int x_get_event(x_t* x, xevent_t* ev) {
	proto_t out;
	proto_init(&out, NULL, 0);

	int res = -1;
	if(fcntl_raw(x->fd, X_CNTL_GET_EVT, NULL, &out) == 0) {
		proto_read_to(&out, ev, sizeof(xevent_t));
		if(ev->type == XEVT_WIN) 
			res = win_event_handle(x, ev);
		else
			res = 0;
	}
	proto_clear(&out);
	return res;
}

int x_is_top(x_t* x) {
	proto_t out;
	proto_init(&out, NULL, 0);

	int res = -1;
	if(fcntl_raw(x->fd, X_CNTL_IS_TOP, NULL, &out) == 0) {
		res = proto_read_int(&out);
	}
	proto_clear(&out);
	return res;
}

int x_screen_info(xscreen_t* scr) {
	int fd = open("/dev/x", O_RDWR);
	if(fd < 0)
		return -1;

	proto_t out;
	proto_init(&out, NULL, 0);
	int ret = fcntl_raw(fd, X_CNTL_SCR_INFO, NULL, &out);
	close(fd);

	if(ret == 0)
		proto_read_to(&out, scr, sizeof(xscreen_t));
	proto_clear(&out);
	return ret;
}

int x_set_visible(x_t* x, bool visible) {
	proto_t in;
	proto_init(&in, NULL, 0);
	proto_add_int(&in, visible);

	int res = fcntl_raw(x->fd, X_CNTL_SET_VISIBLE, &in, NULL);
	proto_clear(&in);
	return res;
}
