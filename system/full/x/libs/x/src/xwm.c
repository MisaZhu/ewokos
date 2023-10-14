#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/proc.h>
#include <sys/vdevice.h>
#include <graph/graph.h>
#include <x/xcntl.h>
#include <x/xwm.h>

#ifdef __cplusplus
extern "C" {
#endif

static int fetch_graph(xwm_t* xwm, void* shm, int w, int h, graph_t* g) {
	(void)xwm;
	uint8_t* g_buf = shm_map(shm);
	if(g_buf == NULL)
		return -1;
	graph_init(g, g_buf, w, h);
	return 0;
}

static void draw_drag_frame(xwm_t* xwm, proto_t* in) {
	grect_t r;
	void* shm = (void*)proto_read_int(in);
	if(shm == NULL)
		return;

	int xw = proto_read_int(in);
	int xh = proto_read_int(in);
	proto_read_to(in, &r, sizeof(grect_t));

	graph_t g;
	if(fetch_graph(xwm, shm, xw, xh, &g) == 0) {
		if(xwm->draw_drag_frame != NULL)
			xwm->draw_drag_frame(&g, &r, xwm->data);
		else
			graph_box(&g, r.x, r.y, r.w, r.h, 0xffffffff);
	}
	shm_unmap(shm);
}

static void draw_frame(xwm_t* xwm, proto_t* in) {
	xinfo_t info;
	void* shm = (void*)proto_read_int(in);
	if(shm == NULL)
		return;

	int xw = proto_read_int(in);
	int xh = proto_read_int(in);
	proto_read_to(in, &info, sizeof(xinfo_t));
	bool top = (proto_read_int(in) != 0);

	if(xw <= 0 || xh <= 0)
		return;

	graph_t g;
	if(fetch_graph(xwm, shm, xw, xh, &g) == 0) {
		grect_t rtitle, rclose, rmax, rmin, rresize;
		memset(&rtitle, 0, sizeof(grect_t));
		memset(&rclose, 0, sizeof(grect_t));
		memset(&rmax, 0, sizeof(grect_t));
		memset(&rmin, 0, sizeof(grect_t));
		memset(&rresize, 0, sizeof(grect_t));

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

		if((info.style & X_STYLE_NO_TITLE) == 0) {
			if(xwm->draw_title != NULL && rtitle.w > 0 && rtitle.h > 0)
				xwm->draw_title(&g, &info, &rtitle, top, xwm->data);

			if((info.style & X_STYLE_NO_RESIZE) == 0) {
				if(xwm->draw_max != NULL && rmax.w > 0 && rmax.h > 0)
					xwm->draw_max(&g, &info, &rmax, top, xwm->data);
				if(xwm->draw_min != NULL && rmin.w > 0 && rmin.h > 0)
					xwm->draw_min(&g, &info, &rmin, top, xwm->data);

				if(info.state != X_STATE_MAX) {
					if(xwm->draw_resize != NULL && rresize.w > 0 && rresize.h > 0)
						xwm->draw_resize(&g, &info, &rresize, top, xwm->data);
				}
			}

			if(xwm->draw_close != NULL && rclose.w > 0 && rclose.h > 0)
				xwm->draw_close(&g, &info, &rclose, top, xwm->data);
		}

		if(xwm->draw_frame != NULL)
			xwm->draw_frame(&g, &info, top, xwm->data);
	}
	shm_unmap(shm);
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

	if((info.style & X_STYLE_NO_TITLE) == 0 &&
			(info.style & X_STYLE_NO_FRAME) == 0) {

		if(xwm->get_title != NULL)
			xwm->get_title(&info, &rtitle, xwm->data);
		if(xwm->get_close != NULL)
			xwm->get_close(&info, &rclose, xwm->data);
		if(xwm->get_min != NULL)
			xwm->get_min(&info, &rmin, xwm->data);

		if((info.style & X_STYLE_NO_RESIZE) == 0) {
			if(xwm->get_max != NULL)
				xwm->get_max(&info, &rmax, xwm->data);
			if(xwm->get_resize != NULL)
				xwm->get_resize(&info, &rresize, xwm->data);
		}
	}
	PF->add(out, &rtitle, sizeof(grect_t))->
		add(out, &rclose, sizeof(grect_t))->
		add(out, &rmin, sizeof(grect_t))->
		add(out, &rmax, sizeof(grect_t))->
		add(out, &rresize, sizeof(grect_t));
}

static void draw_desktop(xwm_t* xwm, proto_t* in) {
	void* shm = (void*)proto_read_int(in);
	int xw = proto_read_int(in);
	int xh = proto_read_int(in);

	graph_t g;
	if(fetch_graph(xwm, shm, xw, xh, &g) == 0) {
		if(xwm->draw_desktop != NULL)
			xwm->draw_desktop(&g, xwm->data);
		shm_unmap(shm);
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
}

void xwm_run(xwm_t* xwm) {
	setenv("XWM", "true");
	dev_cntl("/dev/x", X_DCNTL_SET_XWM, NULL, NULL);
	ipc_serv_run(handle, NULL, xwm, 0);
	while(true) {
		proc_block(getpid(), (uint32_t)xwm_run);
	}
}

#ifdef __cplusplus
}
#endif

