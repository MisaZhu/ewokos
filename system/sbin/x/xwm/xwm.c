#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <graph/graph.h>
#include <sconf.h>
#include <x/xcntl.h>
#include <x/xwm.h>

typedef struct {
	font_t* font;
	uint32_t desk_fg_color;
	uint32_t desk_bg_color;
	uint32_t fg_color;
	uint32_t bg_color;
	uint32_t top_bg_color;
	uint32_t top_fg_color;

	int32_t title_h;
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
		xwm->font = font_by_name(v);

	sconf_free(conf);
	return 0;
}

static void draw_desktop(proto_t* in, proto_t* out) {
	int shm_id = proto_read_int(in);
	int xw = proto_read_int(in);
	int xh = proto_read_int(in);

	void* gbuf = shm_map(shm_id);
	if(gbuf != NULL) {
		graph_t* g = graph_new(gbuf, xw, xh);
		clear(g, _xwm.desk_bg_color);
		//background pattern
		int32_t x, y;
		for(y=10; y<(int32_t)g->h; y+=10) {
			for(x=0; x<(int32_t)g->w; x+=10) {
				pixel(g, x, y, _xwm.desk_fg_color);
			}
		}
		/*
		const char* msg = "(CTRL+TAB to switch to console)";
		draw_text(g, 12, 12, msg, _xwm.font, 0xff000000);
		draw_text(g, 11, 11, msg, _xwm.font, 0xffaaaaaa);
		*/

		graph_free(g);
		shm_unmap(shm_id);
	}
	proto_add_int(out, 0);
}

static void get_frame_rect(xinfo_t* info, grect_t* rect) {
	rect->x = info->r.x;
	rect->y = info->r.y;
	rect->w = info->r.w;
	rect->h = info->r.h;

	if((info->style & X_STYLE_NO_TITLE) == 0) {
		rect->h += _xwm.title_h;
		rect->y -= _xwm.title_h;
	}
}

static void draw_win_frame(graph_t* g, xinfo_t* info, uint32_t fg, uint32_t bg) {
	(void)fg;
	grect_t r;
	get_frame_rect(info, &r);
	box(g, r.x, r.y, r.w, r.h, bg);//win box
}

static void get_title_rect(xinfo_t* info, grect_t* rect) {
	rect->x = info->r.x;
	rect->y = info->r.y - _xwm.title_h;
	rect->w = info->r.w - _xwm.title_h*3;
	rect->h = _xwm.title_h;
}

static void draw_title(graph_t* g, xinfo_t* info, uint32_t fg, uint32_t bg, int8_t top) {
	(void)top;
	grect_t r;
	get_title_rect(info, &r);

	fill(g, r.x, r.y, r.w, _xwm.title_h, bg);//title box
	draw_text(g, r.x+10, r.y+2, info->title, _xwm.font, fg);//title

	gsize_t sz;
	get_text_size(info->title, _xwm.font, &sz);
	int step = 4;
	int y = r.y + step;
	while(1) {
		line(g, r.x+20+sz.w, y, r.x+r.w-10, y, fg);
		y += step;
		if(y >= (r.y + r.h))
			break;
	}
}

static void get_min_rect(xinfo_t* info, grect_t* rect) {
	rect->x = info->r.x+info->r.w-_xwm.title_h*3;
	rect->y = info->r.y - _xwm.title_h;
	rect->w = _xwm.title_h;
	rect->h = _xwm.title_h;
}

static void draw_min(graph_t* g, xinfo_t* info, uint32_t fg, uint32_t bg) {
	(void)bg;
	grect_t r;
	get_min_rect(info, &r);

	fill(g, r.x, r.y, r.w, r.h, bg);
	box(g, r.x+4, r.y+r.h-8, r.w-8, 4, fg);
}

static void get_max_rect(xinfo_t* info, grect_t* rect) {
	rect->x = info->r.x+info->r.w-_xwm.title_h*2;
	rect->y = info->r.y - _xwm.title_h;
	rect->w = _xwm.title_h;
	rect->h = _xwm.title_h;
}

static void draw_max(graph_t* g, xinfo_t* info, uint32_t fg, uint32_t bg) {
	(void)bg;
	grect_t r;
	get_max_rect(info, &r);

	fill(g, r.x, r.y, r.w, r.h, bg);
	box(g, r.x+4, r.y+4, r.w-8, r.h-8, fg);
}

static void get_close_rect(xinfo_t* info, grect_t* rect) {
	rect->x = info->r.x+info->r.w-_xwm.title_h;
	rect->y = info->r.y - _xwm.title_h;
	rect->w = _xwm.title_h;
	rect->h = _xwm.title_h;
}

static void draw_close(graph_t* g, xinfo_t* info, uint32_t fg, uint32_t bg) {
	(void)bg;
	grect_t r;
	get_close_rect(info, &r);

	fill(g, r.x, r.y, r.w, r.h, bg);
	line(g, r.x+4, r.y+4, r.x+r.w-5, r.y+r.h-5, fg);
	line(g, r.x+4, r.y+r.h-5, r.x+r.w-5, r.y+4, fg);
}

static void draw_frame(proto_t* in, proto_t* out) {
	xinfo_t info;
	int shm_id = proto_read_int(in);
	int xw = proto_read_int(in);
	int xh = proto_read_int(in);
	proto_read_to(in, &info, sizeof(xinfo_t));
	int top = proto_read_int(in);

	void* gbuf = shm_map(shm_id);
	if(gbuf != NULL) {
		graph_t* g = graph_new(gbuf, xw, xh);

		uint32_t fg, bg;
		if(top == 0) {
			fg = _xwm.fg_color;
			bg = _xwm.bg_color;
		}
		else {
			fg = _xwm.top_fg_color;
			bg = _xwm.top_bg_color;
		}

		if((info.style & X_STYLE_NO_TITLE) == 0) {
			draw_title(g, &info, fg, bg, top);
			draw_min(g, &info, fg, bg);
			draw_max(g, &info, fg, bg);
			draw_close(g, &info, fg, bg);
		}
		draw_win_frame(g, &info, fg, bg);

		graph_free(g);
		shm_unmap(shm_id);
	}
	proto_add_int(out, 0);
}

static void get_pos(proto_t* in, proto_t* out) {
	xinfo_t info;
	int x = proto_read_int(in);
	int y = proto_read_int(in);
	proto_read_to(in, &info, sizeof(xinfo_t));

	int res = -1;
	if((info.style & X_STYLE_NO_TITLE) == 0 &&
			(info.style & X_STYLE_NO_FRAME) == 0) {
		grect_t rtitle, rclose, rmax;
		get_title_rect(&info, &rtitle);
		get_max_rect(&info, &rmax);
		get_close_rect(&info, &rclose);

		if(check_in_rect(x, y, &rtitle) == 0)
			res = XWM_FRAME_TITLE;
		else if(check_in_rect(x, y, &rclose) == 0)
			res = XWM_FRAME_CLOSE;
		else if(check_in_rect(x, y, &rmax) == 0)
			res = XWM_FRAME_MAX;
	}
	proto_add_int(out, res);
}

static void get_workspace(proto_t* in, proto_t* out) {
	grect_t r;
	int style = proto_read_int(in);
	proto_read_to(in, &r, sizeof(grect_t));

	if((style & X_STYLE_NO_TITLE) == 0 &&
			(style & X_STYLE_NO_FRAME) == 0) {
		r.y += _xwm.title_h;
		r.h -= _xwm.title_h;
	}
	proto_add(out, &r, sizeof(grect_t));
}

void handle(int from_pid, proto_t* in, void* p) {
	(void)p;
	int cmd = proto_read_int(in);
	proto_t out;
	proto_init(&out, NULL, 0);

	if(cmd == XWM_CNTL_DRAW_FRAME) { //draw frame
		draw_frame(in, &out);
	}
	else if(cmd == XWM_CNTL_DRAW_DESKTOP) { //draw desktop
		draw_desktop(in, &out);
	}
	else if(cmd == XWM_CNTL_GET_POS) { //get pos
		get_pos(in, &out);
	}
	else if(cmd == XWM_CNTL_GET_WORKSPACE) { //get workspace
		get_workspace(in, &out);
	}

	ipc_send(from_pid, &out, in->id);
	proto_clear(&out);
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	read_config(&_xwm, "/etc/x/xwm.conf");
	_xwm.title_h = _xwm.font->h+4;
	ipc_server(handle, NULL);
	return 0;
}
