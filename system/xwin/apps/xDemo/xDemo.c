#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <ewoksys/proc.h>
#include <graph/graph_png.h>
#include <graph/graph_ex.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/ipc.h>
#include <font/font.h>
#include <x/x.h>
#include <x/xwin.h>
#include <ewoksys/timer.h>

enum {
	CIRCLE = 0,
	CIRCLE_3D,
	FILL_CIRCLE_3D,
	RECT,
	ARC,
	ROUND,
	ROUND_3D,
	SEMI_ROUND,
	SEMI_FILL_ROUND,
	SEMI_ROUND_3D,
	SEMI_FILL_ROUND_3D,
	RING,
	RING_ARC,
	RING_FILL_ARC,
	MAX_MODE
};

typedef struct {
	int count, fps, imgX, imgY;
	int mode;
	uint32_t tic;
	graph_t* img_big;
	graph_t* img_small;
	x_theme_t theme;
	font_t* font;
} xtest_t;

static xtest_t _xtest_info;

static void xtest_init(void) {
	_xtest_info.tic = 0;
	_xtest_info.fps = 0;
	_xtest_info.count = 0;
	_xtest_info.mode = CIRCLE;
	_xtest_info.imgX = _xtest_info.imgY = 0;

	char buf[128];
	_xtest_info.img_big = png_image_new(x_get_res_name("data/rokid.png", buf, sizeof(buf)-1));	
	_xtest_info.img_small = png_image_new(x_get_res_name("data/rokid_small.png", buf, sizeof(buf)-1));	
	x_get_theme(&_xtest_info.theme);
	_xtest_info.font = font_new(_xtest_info.theme.fontName, true);
}

	
static void xtest_free(void) {
	if(_xtest_info.img_big != NULL)
		graph_free(_xtest_info.img_big);
	if(_xtest_info.img_small != NULL)
		graph_free(_xtest_info.img_small);
}

static void draw_image(graph_t* g, graph_t* img) {
	if(img == NULL)
		return;
	graph_blt_alpha(img, 0, 0, img->w, img->h,
			g, _xtest_info.imgX, _xtest_info.imgY, img->w, img->h, 0xff);
}

static void on_resize(xwin_t* xwin) {
	_xtest_info.tic = 0;
}

static void on_event(xwin_t* xwin, xevent_t* ev) {
	int key = 0;
	if(ev->type == XEVT_IM) {
		key = ev->value.im.value;
		if(key == 27) //esc
			xwin_close(xwin);
	}
}

static void on_repaint(xwin_t* xwin, graph_t* g) {
	int gW = g->w;
	int gH = g->h;
	graph_t* img = gW > (_xtest_info.img_big->w*2) ? _xtest_info.img_big: _xtest_info.img_small;
	uint32_t font_h = _xtest_info.theme.fontSize;

	_xtest_info.count++;

	int x = random_to(gW);
	int y = random_to(gH);
	int w = random_to(gW/4);
	int h = random_to(gH/4);
	int c = rand() | 0x66000000;

	uint32_t low;
	kernel_tic32(NULL, NULL, &low); 
	if(_xtest_info.tic == 0 || (low - _xtest_info.tic) >= 3000000) //3 second
		_xtest_info.tic = low;

	if(w > h*2)
		w = h*2;
	if(h > w*2)
		h = w*2;

	w = w < 32 ? 32 : w;
	h = h < 32 ? 32 : h;


	if(_xtest_info.tic == low) { //3 second
		_xtest_info.fps = _xtest_info.count/3;
		_xtest_info.count = 0;

		_xtest_info.mode++;
		if(_xtest_info.mode >= MAX_MODE)
			_xtest_info.mode = 0;

		graph_fill(g, 0, 0, gW, gH, 0xff000000);
		if(gW > img->w)
			_xtest_info.imgX = random_to(gW - img->w);
		if(gH > (img->h+font_h))
			_xtest_info.imgY = random_to(gH - img->h - font_h);
	}

	if(_xtest_info.mode == CIRCLE) {
		graph_fill_circle(g, x, y, h/2, c);
		graph_circle(g, x, y, h/2+6, 3, c);
	}
	else if(_xtest_info.mode == CIRCLE_3D) {
		graph_circle_3d(g, x, y, h/2, 4, c, true);
	}
	else if(_xtest_info.mode == FILL_CIRCLE_3D) {
		graph_fill_circle_3d(g, x, y, h/2, 4, c, true);
	}
	else if(_xtest_info.mode == ARC) {
		int endangle = rand()%360;
		int start = rand()%360;
		if(endangle == 0)
			endangle = 119;

		graph_fill_arc(g, x, y, h/2, start, endangle, c);
		graph_arc(g, x, y, h/2+4, 2, start, endangle, c);
	}
	else if(_xtest_info.mode == ROUND) {
		graph_fill_round(g, x, y, w, h, 12, c);
		graph_round(g, x-4, y-4, w+8, h+8, 16, 4, c);
	}
	else if(_xtest_info.mode == ROUND_3D) {
		graph_fill_round_3d(g, x, y, w, h, 12, 2, c, true);
	}
	else if(_xtest_info.mode == SEMI_ROUND) {
		// Test semi-round: alternate between top and bottom half
		static bool top_half = true;
		graph_semi_round(g, x, y, w, 20, 3, c, top_half);
		top_half = !top_half;
	}
	else if(_xtest_info.mode == SEMI_FILL_ROUND) {
		// Test semi-fill-round: alternate between top and bottom half
		static bool fill_top_half = true;
		graph_semi_fill_round(g, x, y, w, 20, c, fill_top_half);
		fill_top_half = !fill_top_half;
	}
	else if(_xtest_info.mode == SEMI_ROUND_3D) {
		// Test semi-round-3d: alternate between top and bottom half
		static bool round3d_top_half = true;
		graph_semi_round_3d(g, x, y, w, 20, 3, c, true, round3d_top_half);
		round3d_top_half = !round3d_top_half;
	}
	else if(_xtest_info.mode == SEMI_FILL_ROUND_3D) {
		// Test semi-fill-round-3d: alternate between top and bottom half
		static bool fill3d_top_half = true;
		graph_semi_fill_round_3d(g, x, y, w, 25, 4, c, true, fill3d_top_half);
		fill3d_top_half = !fill3d_top_half;
	}
	else if(_xtest_info.mode == RECT) {
		graph_fill(g, x, y, w, h, c);
		graph_box(g, x-4, y-4, w+8, h+8, c);
	}
	else if(_xtest_info.mode == RING) {
		// Full ring (360 degrees)
		int radius = h/2;
		int thickness = radius/4 + 4;
		graph_ring(g, x, y, radius, thickness, 3, c);
	}
	else if(_xtest_info.mode == RING_ARC) {
		// Ring arc (partial ring) - hollow ring arc
		int radius = h/2;
		int thickness = radius/4 + 4;
		int start = rand()%360;
		int endangle = rand()%360;
		if(start > endangle) {
			int temp = endangle;
			endangle = start;
			start = temp;
		}
		graph_ring_arc(g, x, y, radius, thickness, 3, start, endangle, c);
	}
	else if(_xtest_info.mode == RING_FILL_ARC) {
		// Filled ring arc (pie chart style) - fills from center to outer radius
		int radius = h/2;
		int thickness = radius/4 + 4;
		int endangle = rand()%360;
		int start = rand()%360;
		if(start > endangle) {
			int temp = endangle;
			endangle = start;
			start = temp;
		}
		graph_fill_ring_arc(g, x, y, radius, thickness, start, endangle, c);
	}

	char str[32];
	snprintf(str, 31, "EwokOS FPS: %d", _xtest_info.fps);
	graph_fill(g, _xtest_info.imgX, _xtest_info.imgY+img->h+2, img->w, font_h+4, 0xffffffff);
	graph_draw_text_font(g, _xtest_info.imgX+4, _xtest_info.imgY+img->h+4, str, 
			_xtest_info.font, _xtest_info.theme.fontSize, 0xff000000);
	draw_image(g, img);
}

static bool _repaint = false;
static void loop(void* p) {
	xwin_t* xwin = (xwin_t*)p;
	if(_repaint)
		xwin_repaint(xwin);
	_repaint = false;
	proc_usleep(3000);
}

static void _timerHandler(void) {
	_repaint = true;
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	grect_t desk;
	x_t x;
	x_init(&x, NULL);
	xtest_init();

	x.on_loop = loop;
	xwin_t* xwin = xwin_open(&x, -1, 32, 32, 320, 200, "xDemo", XWIN_STYLE_NORMAL);
	xwin->on_resize = on_resize;
	xwin->on_event = on_event;
	xwin->on_repaint = on_repaint;
	xwin_set_visible(xwin, true);

	int32_t timerID = timer_set(5000, _timerHandler);
	x_run(&x, xwin);
	timer_remove(timerID);
	xwin_destroy(xwin);
	xtest_free();
	return 0;
} 
