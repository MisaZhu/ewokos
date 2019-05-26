#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <kstring.h>
#include <vprintf.h>
#include <kserv.h>
#include <sconf.h>
#include <proto.h>
#include <graph/graph.h>
#include <x/xclient.h>
#include <x/xwm.h>
#include <shm.h>

typedef struct {
	font_t* font;
	uint32_t desk_fg_color;
	uint32_t desk_bg_color;
	uint32_t fg_color;
	uint32_t bg_color;
	uint32_t top_bg_color;
	uint32_t top_fg_color;
} xwm_t;

static xwm_t _xwm;

static int32_t read_config(xwm_t* xwm, const char* fname) {
	sconf_t *conf = sconf_load(fname);	
	if(conf == NULL)
		return -1;

	const char* v = sconf_get(conf, "desk_bg_color");
	if(v[0] != 0) 
		xwm->desk_bg_color = argb_int(atoi_base(v, 16));

	v = sconf_get(conf, "desk_fg_color");
	if(v[0] != 0) 
		xwm->desk_fg_color = argb_int(atoi_base(v, 16));

	v = sconf_get(conf, "bg_color");
	if(v[0] != 0) 
		xwm->bg_color = argb_int(atoi_base(v, 16));


	v = sconf_get(conf, "fg_color");
	if(v[0] != 0) 
		xwm->fg_color = argb_int(atoi_base(v, 16));

	v = sconf_get(conf, "top_bg_color");
	if(v[0] != 0) 
		xwm->top_bg_color = argb_int(atoi_base(v, 16));

	v = sconf_get(conf, "top_fg_color");
	if(v[0] != 0) 
		xwm->top_fg_color = argb_int(atoi_base(v, 16));

	v = sconf_get(conf, "font");
	if(v[0] != 0) 
		xwm->font = get_font_by_name(v);

	sconf_free(conf, MFS);
	return 0;
}

static void draw_background(graph_t* g) {
	clear(g, _xwm.desk_bg_color);
	//background pattern
	int32_t x, y;
	for(y=20; y<(int32_t)g->h; y+=20) {
		for(x=0; x<(int32_t)g->w; x+=20) {
			pixel(g, x, y, _xwm.desk_fg_color);
		}
	}
}

static void draw_frame(graph_t* g, proto_t* in) {
	uint32_t style = (uint32_t)proto_read_int(in);
	const char *title = proto_read_str(in);
	int32_t x = proto_read_int(in);
	int32_t y = proto_read_int(in);
	int32_t w = proto_read_int(in);
	int32_t h = proto_read_int(in);

	if((style & X_STYLE_NO_FRAME) != 0)
		return;

	if((style & X_STYLE_TOP) == 0) {
		box(g, x, y, w, h, _xwm.fg_color);//win box
		fill(g, x, y-20, w, 20, _xwm.bg_color);//title box
		box(g, x, y-20, w, 20, _xwm.fg_color);//title box
		box(g, x+w-20, y-20, 20, 20, _xwm.fg_color);//close box
		draw_text(g, x+2, y-20+2, title, _xwm.font, _xwm.fg_color);//title
	}
	else {
		box(g, x, y, w, h, _xwm.top_fg_color);//win box
		fill(g, x, y-20, w, 20, _xwm.top_bg_color);//title box
		box(g, x, y-20, w, 20, _xwm.top_fg_color);//title box
		box(g, x+w-20, y-20, 20, 20, _xwm.top_fg_color);//close box
		draw_text(g, x+2, y-20+2, title, _xwm.font, _xwm.top_fg_color);//title
	}

	//shadow.
	//fill(g, x+w+1, y+7-20, 6, h+20, 0xff222222);
	//fill(g, x+6, y+h+1, w-5, 6,     0xff222222);
}

static int32_t ipc_call(int32_t pid, int32_t call_id, proto_t* in, proto_t* out, void* p) {
	(void)pid;
	(void)out;
	graph_t* g = (graph_t*)p;

	switch(call_id) {
	case XWM_DRAW_BG:
		draw_background(g);
		return 0;
	case XWM_DRAW_FRAME:
		draw_frame(g, in);
		return 0;
	}
	return -1;
}

static void xwm_init(void) {
	_xwm.font = get_font_by_name("8x16");
	_xwm.top_bg_color = 0xffffff;
	_xwm.top_fg_color = 0x0;
	_xwm.bg_color = 0x888888;
	_xwm.fg_color = 0x0;
	read_config(&_xwm, "/etc/x/xwm.conf");
}

int main(int argc, char* argv[]) {
	xwm_init();

	const char* dev_name = "/dev/X0";
	proto_t* out = proto_new(NULL, 0);
	if(fs_fctrl(dev_name, X_CMD_REG_WM, NULL, out) != 0) {
		proto_free(out);
		return -1;
	}

	int32_t shm_id = proto_read_int(out);
	int32_t w = proto_read_int(out);
	int32_t h = proto_read_int(out);

	proto_free(out);

	void* p = shm_map(shm_id);
	if(p == NULL) {
		shm_unmap(shm_id);
		return -1;
	}

	const char* kserv_name =  "serv.xwin";
	if(argc >= 2)
		kserv_name = argv[1];

	if(kserv_get_by_name(kserv_name) >= 0) {
    printf("Panic: '%s' process has been running already!\n", kserv_name);
		shm_unmap(shm_id);
		return -1;
	}
	
	if(kserv_register(kserv_name) != 0) {
    printf("Panic: '%s' service register failed!\n", kserv_name);
		shm_unmap(shm_id);
		return -1;
	}

	if(kserv_ready() != 0) {
    printf("Panic: '%s' service can not get ready!\n", kserv_name);
		shm_unmap(shm_id);
		return -1;
	}

	graph_t* g = graph_new(p, w, h);
	kserv_run(ipc_call, g, NULL, NULL);
	shm_unmap(shm_id);
	graph_free(g);
	return 0;
}
