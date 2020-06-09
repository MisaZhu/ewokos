#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <vprintf.h>
#include <x/xclient.h>
#include <sys/keydef.h>
#include <upng/upng.h>

static int _alpha = 0xff;

static void draw(graph_t* g, graph_t* img) {
	blt_alpha(img, 0, 0, img->w, img->h,
			g, 0, 0, img->w, img->h, 0xff);
}

static void repaint(x_t* x, graph_t* g) {
	(void)x;
	clear(g, 0xffffff | (_alpha << 24));
	graph_t* img = (graph_t*)x->data;
	draw(g, img);
	draw_text(g, 10, img->h+10, "Press Up/Down to", font_by_name("7x9"), 0xff000000);
	draw_text(g, 10, img->h+20, "change transparency", font_by_name("7x9"), 0xff000000);
	draw_text(g, 10, img->h+30, "of background.", font_by_name("7x9"), 0xff000000);
}

static void event_handle(x_t* x, xevent_t* ev) {
	int key = 0;
	if(ev->type == XEVT_KEYB) {
		key = ev->value.keyboard.value;
		if(key == KEY_ESC)
			x->closed = true;
		else if(key == KEY_UP) {
			_alpha += 0x22;
			if(_alpha > 0xff)
				_alpha = 0xff;
			x_repaint(x);
		}
		else if(key == KEY_DOWN) {
			_alpha -= 0x22;
			if(_alpha < 0)
				_alpha = 0x0;
			x_repaint(x);
		}
	}
}

int main(int argc, char* argv[]) {
	if(argc < 2) {
		printf("Usage: png <png filename>\n");
		return -1;
	}

	//printf("loading...");
	graph_t* img = png_image_new(argv[1]);
	if(img == NULL)  {
		//printf("open '%s' error!\n", argv[1]);
		return -1;
	}
	//printf("ok\n");

	_alpha = 0xff;
	x_t* x = x_open(10, 10, img->w, img->h+60, "png", X_STYLE_NORMAL | X_STYLE_NO_RESIZE | X_STYLE_ALPHA);
	if(x == NULL) {
		graph_free(img);
		return -1;
	}
	x->data = img;
	x->on_repaint = repaint;
	x->on_event = event_handle;
	x_set_visible(x, true);
	x_run(x);

	graph_free(img);
	x_close(x);
	return 0;
}

