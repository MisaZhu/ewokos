#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/ipc.h>
#include <sys/shm.h>
#include <ewoksys/proc.h>
#include <ewoksys/vdevice.h>
#include <graph/graph.h>
#include <x/xcntl.h>
#include <x/xwm.h>
#include <setenv.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int shm_id;
	int w;
	int h;
	uint8_t* g;
} xwm_graph_t;

static xwm_graph_t _xwm_graph;

static bool fetch_desktop_graph(xwm_t* xwm, int32_t shm_id, int w, int h, graph_t* g) {
	(void)xwm;
	uint8_t* g_buf = NULL;
	if(w == _xwm_graph.w && h == _xwm_graph.h && shm_id == _xwm_graph.shm_id)
		g_buf = _xwm_graph.g;
	else {
		if(_xwm_graph.g != NULL) {
			shmdt(_xwm_graph.g);
			_xwm_graph.g = NULL;
		}
		g_buf = shmat(shm_id, 0, 0);
		_xwm_graph.g = g_buf;
		_xwm_graph.w = w;
		_xwm_graph.h = h;
		_xwm_graph.shm_id = shm_id;
	}

	if(g_buf == NULL)
		return false;
	graph_init(g, (const uint32_t*)g_buf, w, h);
	return true;
}

static bool fetch_frame_graph(xwm_t* xwm, xinfo_t* info, graph_t* g) {
	if(info == NULL || info->frame_g_shm_id == -1)
		return false;
	memset(g, 0, sizeof(graph_t));

	uint32_t* frame_g_shm = (uint32_t*)shmat(info->frame_g_shm_id, 0, 0);
	if(frame_g_shm == NULL)
		return false;
	g->buffer = frame_g_shm;
	g->w = info->winr.w;
	g->h = info->winr.h;
	g->need_free = false;
	return true;
}

static bool fetch_ws_graph(xwm_t* xwm, xinfo_t* info, graph_t* g) {
	if(info == NULL || info->ws_g_shm_id == -1)
		return false;
	memset(g, 0, sizeof(graph_t));

	uint32_t* ws_g_shm = (uint32_t*)shmat(info->ws_g_shm_id, 0, 0);
	if(ws_g_shm == NULL)
		return false;
	g->buffer = ws_g_shm;
	g->w = info->wsr.w;
	g->h = info->wsr.h;
	g->need_free = false;
	return true;
}

static void free_win_graph(graph_t* g) {
	if(g->buffer == NULL)
		return;
	shmdt(g->buffer);
}

static void draw_drag_frame(xwm_t* xwm, proto_t* in) {
	grect_t r;
	int32_t shm_id = proto_read_int(in);
	if(shm_id == -1)
		return;

	int xw = proto_read_int(in);
	int xh = proto_read_int(in);
	proto_read_to(in, &r, sizeof(grect_t));

	graph_t g;
	if(fetch_desktop_graph(xwm, shm_id, xw, xh, &g)) {
		if(xwm->draw_drag_frame != NULL)
			xwm->draw_drag_frame(&g, &r, xwm->data);
		else
			graph_box(&g, r.x, r.y, r.w, r.h, 0xffffffff);
	}
}

static void draw_frame(xwm_t* xwm, proto_t* in) {
	xinfo_t info;
	int32_t shm_id = proto_read_int(in);
	if(shm_id == -1)
		return;

	int xw = proto_read_int(in);
	int xh = proto_read_int(in);
	proto_read_to(in, &info, sizeof(xinfo_t));
	bool bg_effect = (proto_read_int(in) != 0);
	bool top = info.focused;

	if(xw <= 0 || xh <= 0)
		return;

	graph_t desktop_g;
	graph_t frame_g;
	graph_t ws_g;
	if(!fetch_desktop_graph(xwm, shm_id, xw, xh, &desktop_g))
		return;
	if(!fetch_frame_graph(xwm, &info, &frame_g))
		return;
	if(!fetch_ws_graph(xwm, &info, &ws_g))
		return;
	
	if((info.style & XWIN_STYLE_NO_FRAME) == 0) {
		grect_t rtitle, rclose, rmax, rmin, rresize, rframe;
		memset(&rtitle, 0, sizeof(grect_t));
		memset(&rclose, 0, sizeof(grect_t));
		memset(&rmax, 0, sizeof(grect_t));
		memset(&rmin, 0, sizeof(grect_t));
		memset(&rresize, 0, sizeof(grect_t));
		memset(&rframe, 0, sizeof(grect_t));

		if(xwm->get_title != NULL)
			xwm->get_title(&info, &rtitle, xwm->data);
		if(xwm->get_close != NULL)
			xwm->get_close(&info, &rclose, xwm->data);
		if(xwm->get_max != NULL)
			xwm->get_max(&info, &rmax, xwm->data);
		if(xwm->get_min != NULL)
			xwm->get_min(&info, &rmin, xwm->data);
		if(xwm->get_resize != NULL)
			xwm->get_resize(&info, &rresize, xwm->data);
		if(xwm->get_frame != NULL)
			xwm->get_frame(&info, &rframe, xwm->data);

		if((info.style & XWIN_STYLE_NO_TITLE) == 0) {
			if(xwm->draw_title != NULL && rtitle.w > 0 && rtitle.h > 0)
				xwm->draw_title(&desktop_g, &frame_g, &info, &rtitle, top, xwm->data);

			if((info.style & XWIN_STYLE_NO_RESIZE) == 0) {
				if(xwm->draw_max != NULL && rmax.w > 0 && rmax.h > 0)
					xwm->draw_max(&frame_g, &info, &rmax, top, xwm->data);
				if(xwm->draw_min != NULL && rmin.w > 0 && rmin.h > 0)
					xwm->draw_min(&frame_g, &info, &rmin, top, xwm->data);
			}

			if(xwm->draw_close != NULL && rclose.w > 0 && rclose.h > 0)
				xwm->draw_close(&frame_g, &info, &rclose, top, xwm->data);
		}

		if(info.state != XWIN_STATE_MAX) {
			if(xwm->draw_frame != NULL)
				xwm->draw_frame(&desktop_g, &frame_g, &ws_g, &info, &rframe, top, xwm->data);

			if((info.style & XWIN_STYLE_NO_TITLE) == 0) {
				if((info.style & XWIN_STYLE_NO_RESIZE) == 0) {
					if(xwm->draw_resize != NULL && rresize.w > 0 && rresize.h > 0)
						xwm->draw_resize(&frame_g, &info, &rresize, top, xwm->data);
				}
			}

			if(xwm->draw_shadow!= NULL && xwm->theme.shadow > 0)
				xwm->draw_shadow(&desktop_g, &frame_g, &info, top, xwm->data);
		}
	}

	if((info.style & XWIN_STYLE_NO_BG_EFFECT) == 0 &&
			!info.focused &&
			xwm->draw_bg_effect != NULL) {
		xwm->draw_bg_effect(&desktop_g, &frame_g, &ws_g, &info, bg_effect, xwm->data);
	}
	free_win_graph(&frame_g);
	free_win_graph(&ws_g);
}

static void get_frame_areas(xwm_t* xwm, proto_t* in, proto_t* out) {
	xinfo_t info;
	proto_read_to(in, &info, sizeof(xinfo_t));

	grect_t rtitle, rclose, rmax, rmin, rresize;
	memset(&rtitle, 0, sizeof(grect_t));
	memset(&rclose, 0, sizeof(grect_t));
	memset(&rmax, 0, sizeof(grect_t));
	memset(&rmin, 0, sizeof(grect_t));
	memset(&rresize, 0, sizeof(grect_t));

	if((info.style & XWIN_STYLE_NO_TITLE) == 0 &&
			(info.style & XWIN_STYLE_NO_FRAME) == 0) {

		if(xwm->get_title != NULL)
			xwm->get_title(&info, &rtitle, xwm->data);
		if(xwm->get_close != NULL)
			xwm->get_close(&info, &rclose, xwm->data);
		if(xwm->get_min != NULL)
			xwm->get_min(&info, &rmin, xwm->data);

		if((info.style & XWIN_STYLE_NO_RESIZE) == 0) {
			if(xwm->get_max != NULL)
				xwm->get_max(&info, &rmax, xwm->data);
			if(xwm->get_resize != NULL)
				xwm->get_resize(&info, &rresize, xwm->data);
		}
	}

	rtitle.x += info.winr.x;
	rtitle.y += info.winr.y;
	rclose.x += info.winr.x;
	rclose.y += info.winr.y;
	rmin.x += info.winr.x;
	rmin.y += info.winr.y;
	rmax.x += info.winr.x;
	rmax.y += info.winr.y;
	rresize.x += info.winr.x;
	rresize.y += info.winr.y;

	PF->add(out, &rtitle, sizeof(grect_t))->
		add(out, &rclose, sizeof(grect_t))->
		add(out, &rmin, sizeof(grect_t))->
		add(out, &rmax, sizeof(grect_t))->
		add(out, &rresize, sizeof(grect_t));
}

static void draw_desktop(xwm_t* xwm, proto_t* in) {
	int32_t shm_id = proto_read_int(in);
	if(shm_id == -1)
		return;

	int xw = proto_read_int(in);
	int xh = proto_read_int(in);

	graph_t g;
	if(fetch_desktop_graph(xwm, shm_id, xw, xh, &g)) {
		if(xwm->draw_desktop != NULL)
			xwm->draw_desktop(&g, xwm->data);
	}
}

static void get_win_space(xwm_t* xwm, proto_t* in, proto_t* out) {
	grect_t r, winr;
	int style = proto_read_int(in);
	proto_read_to(in, &r, sizeof(grect_t));
	memcpy(&winr, &r, sizeof(grect_t));
	
	if(xwm->get_win_space != NULL)
		xwm->get_win_space(style, &r, &winr, xwm->data);

	PF->add(out, &winr, sizeof(grect_t));
}

static void get_min_size(xwm_t* xwm, proto_t* in, proto_t* out) {
	xinfo_t info;
	proto_read_to(in, &info, sizeof(xinfo_t));
	
	int w = 0, h = 0;
	if(xwm->get_min_size != NULL)
		xwm->get_min_size(&info, &w, &h, xwm->data);

	PF->addi(out, w)->
		addi(out, h);
}

static void set_theme(xwm_t* xwm, proto_t* in, proto_t* out) {
	proto_read_to(in, &xwm->theme, sizeof(xwm_theme_t));
	if(xwm->update_theme != NULL)
		xwm->update_theme(false, xwm->data);
}

static void handle(int from_pid, int cmd, proto_t* in, proto_t* out, void* p) {
	(void)from_pid;
	xwm_t* xwm = (xwm_t*)p;

	if(cmd == XWM_CNTL_DRAW_FRAME) { //draw frame
		draw_frame(xwm, in);
	}
	else if(cmd == XWM_CNTL_DRAW_DRAG_FRAME) {
		draw_drag_frame(xwm, in);
	}
	else if(cmd == XWM_CNTL_DRAW_DESKTOP) { //draw desktop
		draw_desktop(xwm, in);
	}
	else if(cmd == XWM_CNTL_GET_FRAME_AREAS) {
		get_frame_areas(xwm, in, out);
	}
	else if(cmd == XWM_CNTL_GET_WIN_SPACE) { //get workspace
		get_win_space(xwm, in, out);
	}
	else if(cmd == XWM_CNTL_GET_MIN_SIZE) {
		get_min_size(xwm, in, out);
	}
	else if(cmd == XWM_CNTL_SET_THEME) {
		set_theme(xwm, in, out);
	}
}

void xwm_run(xwm_t* xwm) {
	memset(&_xwm_graph, 0, sizeof(xwm_graph_t));
	ipc_serv_run(handle, NULL, xwm, IPC_NON_BLOCK);

	setenv("XWM", "true");
	dev_cntl("/dev/x", X_DCNTL_SET_XWM, NULL, NULL);

	while(true) {
		proc_block_by(getpid(), (uint32_t)xwm_run);
	}
	
	if(_xwm_graph.g != NULL) {
		shmdt(_xwm_graph.g);
	}
}


#ifdef __cplusplus
}
#endif

