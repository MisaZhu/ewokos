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

typedef struct {
	font_t* font;
	uint32_t desk_fg_color;
	uint32_t desk_bg_color;
	uint32_t fg_color;
	uint32_t bg_color;
	uint32_t top_bg_color;
	uint32_t top_fg_color;

	int32_t title_h;
} xwm_conf_t;

static xwm_conf_t _xwm_config;

static int32_t read_config(xwm_conf_t* xwm, const char* fname) {
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

/*-------get area functions.----------*/

static void get_workspace(int style, grect_t* xr, grect_t* wsr, void* p) {
	(void)p;
	wsr->x = xr->x;
	wsr->w = xr->w;

	if((style & X_STYLE_NO_TITLE) == 0 &&
			(style & X_STYLE_NO_FRAME) == 0) {
		wsr->y = xr->y + _xwm_config.title_h;
		wsr->h = xr->h - _xwm_config.title_h;
	}
}

static void get_title(xinfo_t* info, grect_t* rect, void* p) {
	(void)p;
	rect->x = info->r.x;
	rect->y = info->r.y - _xwm_config.title_h;

	if((info->style & X_STYLE_NO_RESIZE) == 0)
		rect->w = info->r.w - _xwm_config.title_h*3;
	else
		rect->w = info->r.w - _xwm_config.title_h;

	rect->h = _xwm_config.title_h;
}

static void get_min(xinfo_t* info, grect_t* rect, void* p) {
	(void)p;
	rect->x = info->r.x+info->r.w-_xwm_config.title_h*3;
	rect->y = info->r.y - _xwm_config.title_h;
	rect->w = _xwm_config.title_h;
	rect->h = _xwm_config.title_h;
}

static void get_max(xinfo_t* info, grect_t* rect, void* p) {
	(void)p;
	rect->x = info->r.x+info->r.w-_xwm_config.title_h*2;
	rect->y = info->r.y - _xwm_config.title_h;
	rect->w = _xwm_config.title_h;
	rect->h = _xwm_config.title_h;
}

static void get_close(xinfo_t* info, grect_t* rect, void* p) {
	(void)p;
	rect->x = info->r.x+info->r.w-_xwm_config.title_h;
	rect->y = info->r.y - _xwm_config.title_h;
	rect->w = _xwm_config.title_h;
	rect->h = _xwm_config.title_h;
}

/*-------draw functions.----------*/
static void draw_frame(graph_t* g, xinfo_t* info, bool top, void* p) {
	(void)p;
	(void)info;
	uint32_t bg;
	if(!top) {
		bg = _xwm_config.bg_color;
	}
	else {
		bg = _xwm_config.top_bg_color;
	}

	int x = info->r.x;
	int y = info->r.y;
	int w = info->r.w;
	int h = info->r.h;

	if((info->style & X_STYLE_NO_TITLE) == 0) {
		h += _xwm_config.title_h;
		y -= _xwm_config.title_h;
	}
	box(g, x, y, w, h, bg);//win box
}

static void draw_title(graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p) {
	(void)p;
	uint32_t fg, bg;
	if(!top) {
		fg = _xwm_config.fg_color;
		bg = _xwm_config.bg_color;
	}
	else {
		fg = _xwm_config.top_fg_color;
		bg = _xwm_config.top_bg_color;
	}

	fill(g, r->x, r->y, r->w, _xwm_config.title_h, bg);//title box
	draw_text(g, r->x+10, r->y+2, info->title, _xwm_config.font, fg);//title

	gsize_t sz;
	get_text_size(info->title, _xwm_config.font, &sz);
	int step = 4;
	int y = r->y + step;
	while(1) {
		line(g, r->x+20+sz.w, y, r->x+r->w-10, y, fg);
		y += step;
		if(y >= (r->y + r->h))
			break;
	}
}

static void draw_min(graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p) {
	(void)p;
	(void)info;
	uint32_t fg, bg;
	if(!top) {
		fg = _xwm_config.fg_color;
		bg = _xwm_config.bg_color;
	}
	else {
		fg = _xwm_config.top_fg_color;
		bg = _xwm_config.top_bg_color;
	}

	fill(g, r->x, r->y, r->w, r->h, bg);
	box(g, r->x+4, r->y+r->h-8, r->w-8, 4, fg);
}

static void draw_max(graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p) {
	(void)p;
	(void)info;
	uint32_t fg, bg;
	if(!top) {
		fg = _xwm_config.fg_color;
		bg = _xwm_config.bg_color;
	}
	else {
		fg = _xwm_config.top_fg_color;
		bg = _xwm_config.top_bg_color;
	}

	fill(g, r->x, r->y, r->w, r->h, bg);
	box(g, r->x+4, r->y+4, r->w-8, r->h-8, fg);
}

static void draw_close(graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p) {
	(void)info;
	(void)p;
	uint32_t fg, bg;
	if(!top) {
		fg = _xwm_config.fg_color;
		bg = _xwm_config.bg_color;
	}
	else {
		fg = _xwm_config.top_fg_color;
		bg = _xwm_config.top_bg_color;
	}

	fill(g, r->x, r->y, r->w, r->h, bg);
	line(g, r->x+4, r->y+4, r->x+r->w-5, r->y+r->h-5, fg);
	line(g, r->x+4, r->y+r->h-5, r->x+r->w-5, r->y+4, fg);
}

static void draw_desktop(graph_t* g, void* p) {
	(void)p;
	clear(g, _xwm_config.desk_bg_color);
	//background pattern
	int32_t x, y;
	for(y=10; y<(int32_t)g->h; y+=10) {
		for(x=0; x<(int32_t)g->w; x+=10) {
			pixel(g, x, y, _xwm_config.desk_fg_color);
		}
	}
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	xwm_t xwm;
	xwm.get_workspace = get_workspace;
	xwm.get_close = get_close;
	xwm.get_max = get_max;
	xwm.get_min = get_min;
	xwm.get_title = get_title;

	xwm.draw_frame = draw_frame;
	xwm.draw_close = draw_close;
	xwm.draw_title = draw_title;
	xwm.draw_min = draw_min;
	xwm.draw_max = draw_max;
	xwm.draw_desktop = draw_desktop;

	read_config(&_xwm_config, "/etc/x/xwm_macos7.conf");
	_xwm_config.title_h = _xwm_config.font->h+4;

	xwm_run(&xwm);
	return 0;
}
