#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <ewoksys/proc.h>
#include <graph/graph_png.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/ipc.h>
#include <font/font.h>
#include <x/x.h>
#include <x/xwin.h>
#include <ewoksys/timer.h>

static const int CIRCLE = 0;
static const int RECT   = 1;
static const int ARC  = 2;
static const int ROUND  = 3;

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
	int c = rand();

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
		if(_xtest_info.mode > ROUND)
			_xtest_info.mode = 0;

		graph_fill(g, 0, 0, gW, gH, 0xff000000);
		if(gW > img->w)
			_xtest_info.imgX = random_to(gW - img->w);
		if(gH > (img->h+font_h))
			_xtest_info.imgY = random_to(gH - img->h - font_h);
	}

	if(_xtest_info.mode == CIRCLE) {
		graph_fill_circle(g, x, y, h/2, c);
		graph_circle(g, x, y, h/2+4, c);
	}
	else if(_xtest_info.mode == ARC) {
		int endangle = c%5 * 44;
		if(endangle == 0)
			endangle = 119;
		graph_fill_arc(g, x, y, h/2, 0, endangle, c);
		graph_arc(g, x, y, h/2+2, 0, endangle, c);
	}
	else if(_xtest_info.mode == ROUND) {
		graph_fill_round(g, x, y, w, h, 12, c);
		graph_round(g, x-4, y-4, w+8, h+8, 16, c);
	}
	else if(_xtest_info.mode == RECT) {
		graph_fill(g, x, y, w, h, c);
		graph_box(g, x-4, y-4, w+8, h+8, c);
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
	xwin_t* xwin = xwin_open(&x, -1, 32, 32, 320, 200, "xtest", XWIN_STYLE_NORMAL);
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
