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

static int32_t do_config(xwm_conf_t* xwm) {
	xwm->desk_bg_color = 0xff666666;
	xwm->desk_fg_color = 0xffaaaaaa;

	xwm->bg_color = 0xff888888;
	xwm->fg_color = 0xff444444;

	xwm->top_bg_color = 0xffdddddd;
	xwm->top_fg_color = 0xff222222;

	xwm->font = font_by_name("8x16");
	return 0;
}

static void get_frame_rect(xinfo_t* info, grect_t* rect) {
	rect->x = info->r.x;
	rect->y = info->r.y;
	rect->w = info->r.w;
	rect->h = info->r.h;

	if((info->style & X_STYLE_NO_TITLE) == 0) {
		rect->h += _xwm_config.title_h;
		rect->y -= _xwm_config.title_h;
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
	rect->y = info->r.y - _xwm_config.title_h;

	if((info->style & X_STYLE_NO_RESIZE) == 0)
		rect->w = info->r.w - _xwm_config.title_h*3;
	else
		rect->w = info->r.w - _xwm_config.title_h;

	rect->h = _xwm_config.title_h;
}

static void draw_title(graph_t* g, xinfo_t* info, uint32_t fg, uint32_t bg, int8_t top) {
	(void)top;
	grect_t r;
	get_title_rect(info, &r);

	fill(g, r.x, r.y, r.w, _xwm_config.title_h, bg);//title box
	draw_text(g, r.x+10, r.y+2, info->title, _xwm_config.font, fg);//title
}

static void get_min_rect(xinfo_t* info, grect_t* rect) {
	rect->x = info->r.x+info->r.w-_xwm_config.title_h*3;
	rect->y = info->r.y - _xwm_config.title_h;
	rect->w = _xwm_config.title_h;
	rect->h = _xwm_config.title_h;
}

static void draw_min(graph_t* g, xinfo_t* info, uint32_t fg, uint32_t bg) {
	(void)bg;
	grect_t r;
	get_min_rect(info, &r);

	fill(g, r.x, r.y, r.w, r.h, bg);
	box(g, r.x+4, r.y+r.h-8, r.w-8, 4, fg);
}

static void get_max_rect(xinfo_t* info, grect_t* rect) {
	rect->x = info->r.x+info->r.w-_xwm_config.title_h*2;
	rect->y = info->r.y - _xwm_config.title_h;
	rect->w = _xwm_config.title_h;
	rect->h = _xwm_config.title_h;
}

static void draw_max(graph_t* g, xinfo_t* info, uint32_t fg, uint32_t bg) {
	(void)bg;
	grect_t r;
	get_max_rect(info, &r);

	fill(g, r.x, r.y, r.w, r.h, bg);
	box(g, r.x+4, r.y+4, r.w-8, r.h-8, fg);
}

static void get_close_rect(xinfo_t* info, grect_t* rect) {
	rect->x = info->r.x+info->r.w-_xwm_config.title_h;
	rect->y = info->r.y - _xwm_config.title_h;
	rect->w = _xwm_config.title_h;
	rect->h = _xwm_config.title_h;
}

static void draw_close(graph_t* g, xinfo_t* info, uint32_t fg, uint32_t bg) {
	(void)bg;
	grect_t r;
	get_close_rect(info, &r);

	fill(g, r.x, r.y, r.w, r.h, bg);
	line(g, r.x+4, r.y+4, r.x+r.w-5, r.y+r.h-5, fg);
	line(g, r.x+4, r.y+r.h-5, r.x+r.w-5, r.y+4, fg);
}

static void draw_frame(graph_t* g, xinfo_t* info, bool top, void* p) {
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

	if((info->style & X_STYLE_NO_TITLE) == 0) {
		draw_title(g, info, fg, bg, top);
		if((info->style & X_STYLE_NO_RESIZE) == 0) {
			draw_min(g, info, fg, bg);
			draw_max(g, info, fg, bg);
		}
		draw_close(g, info, fg, bg);
	}
	draw_win_frame(g, info, fg, bg);
}

static int get_pos(int x, int y, xinfo_t* info, void* p) {
	(void)p;
	int res = -1;
	if((info->style & X_STYLE_NO_TITLE) == 0 &&
			(info->style & X_STYLE_NO_FRAME) == 0) {
		grect_t rtitle, rclose, rmax;
		get_title_rect(info, &rtitle);
		get_close_rect(info, &rclose);

		if(check_in_rect(x, y, &rtitle) == 0)
			res = XWM_FRAME_TITLE;
		else if(check_in_rect(x, y, &rclose) == 0)
			res = XWM_FRAME_CLOSE;
		else if((info->style & X_STYLE_NO_RESIZE) == 0) {
			get_max_rect(info, &rmax);
			if(check_in_rect(x, y, &rmax) == 0)
				res = XWM_FRAME_MAX;
		}
	}
	return res;
}

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

static void draw_desktop(graph_t* g, void* p) {
	(void)p;
	clear(g, _xwm_config.desk_bg_color);
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	xwm_t xwm;
	xwm.get_workspace = get_workspace;
	xwm.get_pos = get_pos;
	xwm.draw_frame = draw_frame;
	xwm.draw_desktop = draw_desktop;

	do_config(&_xwm_config);
	_xwm_config.title_h = _xwm_config.font->h+4;

	xwm_run(&xwm);
	return 0;
}
