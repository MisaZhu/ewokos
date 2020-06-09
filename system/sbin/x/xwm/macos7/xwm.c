#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/proc.h>
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
	rect->x = info->wsr.x + _xwm_config.title_h;
	rect->y = info->wsr.y - _xwm_config.title_h;

	if((info->style & X_STYLE_NO_RESIZE) == 0)
		rect->w = info->wsr.w - _xwm_config.title_h*2;
	else
		rect->w = info->wsr.w - _xwm_config.title_h;

	rect->h = _xwm_config.title_h;
}

static void get_min(xinfo_t* info, grect_t* rect, void* p) {
	(void)p;
	rect->x = info->wsr.x+info->wsr.w-_xwm_config.title_h*2;
	rect->y = info->wsr.y - _xwm_config.title_h;
	rect->w = _xwm_config.title_h;
	rect->h = _xwm_config.title_h;
}

static void get_max(xinfo_t* info, grect_t* rect, void* p) {
	(void)p;
	rect->x = info->wsr.x+info->wsr.w-_xwm_config.title_h*1;
	rect->y = info->wsr.y - _xwm_config.title_h;
	rect->w = _xwm_config.title_h;
	rect->h = _xwm_config.title_h;
}

static void get_close(xinfo_t* info, grect_t* rect, void* p) {
	(void)p;
	rect->x = info->wsr.x;
	rect->y = info->wsr.y - _xwm_config.title_h;
	rect->w = _xwm_config.title_h;
	rect->h = _xwm_config.title_h;
}

/*-------draw functions.----------*/
static void get_color(uint32_t *fg, uint32_t* bg, bool top) {
	if(!top) {
		*fg = _xwm_config.fg_color;
		*bg = _xwm_config.bg_color;
	}
	else {
		*fg = _xwm_config.top_fg_color;
		*bg = _xwm_config.top_bg_color;
	}
}

static void draw_frame(graph_t* g, xinfo_t* info, bool top, void* p) {
	(void)p;
	(void)info;
	uint32_t fg, bg;
	get_color(&fg, &bg, top);

	int x = info->wsr.x;
	int y = info->wsr.y;
	int w = info->wsr.w;
	int h = info->wsr.h;
	//int h = 0;

	if((info->style & X_STYLE_NO_TITLE) == 0) {
		h += _xwm_config.title_h;
		//h = _xwm_config.title_h;
		y -= _xwm_config.title_h;
	}
	box(g, x, y, w, h, bg);//win box
}

static void draw_title_pattern(graph_t* g, int x, int y, int w, int h, uint32_t fg, uint32_t bg) {
	(void)bg;
	int step = 3;
	y = y + step;
	int steps = h / step;

	for(int i=0; i< steps; i++) {
		line(g, x+4, y, x+w-8, y, fg);
		y += step;
	}
}

static void draw_title(graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p) {
	(void)p;
	uint32_t fg, bg;
	get_color(&fg, &bg, top);

	gsize_t sz;
	get_text_size(info->title, _xwm_config.font, &sz);

	int pw = (r->w-sz.w)/2;
	fill(g, r->x, r->y, r->w, _xwm_config.title_h, bg);//title box
	if(top) {
		draw_title_pattern(g, r->x, r->y, pw, r->h, fg, bg);
		draw_title_pattern(g, r->x+pw+sz.w, r->y, pw, r->h, fg, bg);
	}
	draw_text(g, r->x+pw, r->y+2, info->title, _xwm_config.font, fg);//title
}

static void draw_min(graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p) {
	(void)p;
	(void)info;
	uint32_t fg, bg;
	get_color(&fg, &bg, top);

	fill(g, r->x, r->y, r->w, r->h, bg);
	box(g, r->x+2, r->y+r->h-6, r->w-4, 4, fg);
}

static void draw_max(graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p) {
	(void)p;
	(void)info;
	uint32_t fg, bg;
	get_color(&fg, &bg, top);

	fill(g, r->x, r->y, r->w, r->h, bg);
	box(g, r->x+2, r->y+2, r->w-4, r->h-4, fg);
	box(g, r->x+2, r->y+2, r->w-8, r->h-8, fg);
}

static void draw_close(graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p) {
	(void)info;
	(void)p;
	uint32_t fg, bg;
	get_color(&fg, &bg, top);

	fill(g, r->x, r->y, r->w, r->h, bg);
	box(g, r->x+2, r->y+2, r->w-4, r->h-4, fg);
}

static void draw_desktop(graph_t* g, void* p) {
	(void)p;
	clear(g, _xwm_config.desk_bg_color);
	//background pattern
	/*int32_t x, y;
	for(y=10; y<(int32_t)g->h; y+=10) {
		for(x=0; x<(int32_t)g->w; x+=10) {
			pixel(g, x, y, _xwm_config.desk_fg_color);
		}
	}
	*/
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
