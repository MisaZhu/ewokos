#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/proc.h>
#include <sys/kserv.h>
#include <graph/graph.h>
#include <sconf.h>
#include <x/xcntl.h>
#include <x/xwm.h>

static void draw_frame(xwm_t* xwm, proto_t* in, proto_t* out) {
	xinfo_t info;
	int shm_id = proto_read_int(in);
	int xw = proto_read_int(in);
	int xh = proto_read_int(in);
	proto_read_to(in, &info, sizeof(xinfo_t));
	int top = proto_read_int(in);

	void* gbuf = shm_map(shm_id);
	if(gbuf != NULL) {
		graph_t* g = graph_new(gbuf, xw, xh);

		if(xwm->draw_frame != NULL)
			xwm->draw_frame(g, &info, top != 0, xwm->data);

		graph_free(g);
		shm_unmap(shm_id);
	}
	PF->addi(out, 0);
}

static void get_pos(xwm_t* xwm, proto_t* in, proto_t* out) {
	xinfo_t info;
	int x = proto_read_int(in);
	int y = proto_read_int(in);
	proto_read_to(in, &info, sizeof(xinfo_t));

	int res = -1;
	if(xwm->get_pos != NULL)
		res = xwm->get_pos(x, y, &info, xwm->data);

	PF->addi(out, res);
}

static void draw_desktop(xwm_t* xwm, proto_t* in, proto_t* out) {
	int shm_id = proto_read_int(in);
	int xw = proto_read_int(in);
	int xh = proto_read_int(in);

	void* gbuf = shm_map(shm_id);
	if(gbuf != NULL) {
		graph_t* g = graph_new(gbuf, xw, xh);
		if(xwm->draw_desktop != NULL)
			xwm->draw_desktop(g, xwm->data);
		graph_free(g);
		shm_unmap(shm_id);
	}
	PF->addi(out, 0);
}

static void get_workspace(xwm_t* xwm, proto_t* in, proto_t* out) {
	grect_t r, wsr;
	int style = proto_read_int(in);
	proto_read_to(in, &r, sizeof(grect_t));
	memcpy(&wsr, &r, sizeof(grect_t));
	
	if(xwm->get_workspace != NULL)
		xwm->get_workspace(style, &r, &wsr, xwm->data);

	PF->add(out, &wsr, sizeof(grect_t));
}

static void handle(int from_pid, int cmd, proto_t* in, proto_t* out, void* p) {
	(void)from_pid;
	xwm_t* xwm = (xwm_t*)p;

	if(cmd == XWM_CNTL_DRAW_FRAME) { //draw frame
		draw_frame(xwm, in, out);
	}
	else if(cmd == XWM_CNTL_DRAW_DESKTOP) { //draw desktop
		draw_desktop(xwm, in, out);
	}
	else if(cmd == XWM_CNTL_GET_POS) { //get pos
		get_pos(xwm, in, out);
	}
	else if(cmd == XWM_CNTL_GET_WORKSPACE) { //get workspace
		get_workspace(xwm, in, out);
	}
}

void xwm_run(xwm_t* xwm) {
	kserv_run(handle, xwm, false);
	while(true) {
		sleep(1);
	}
}
