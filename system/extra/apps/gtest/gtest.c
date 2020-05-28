#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <vprintf.h>
#include <x/xclient.h>

static bool top = false;
static void on_focus(x_t* x) {
	(void)x;
	top = true;
}

static void on_unfocus(x_t* x) {
	(void)x;
	top = false;
}

static void event_handle(x_t* x, xevent_t* ev) {
	int key = 0;
	if(ev->type == XEVT_KEYB) {
		key = ev->value.keyboard.value;
		if(key == 27) //esc
			x->closed = true;
	}
}

static void repaint(x_t* x, graph_t* g) {
	int* i = (int*)x->data;
	char str[32];
	font_t* font = font_by_name("12x24");
	snprintf(str, 31, "paint = %d", (*i)++);
	clear(g, argb_int(0xff0000ff));
	draw_text(g, 10, 10, str, font, 0xffffffff);
}

static void step(x_t* x) {
	if(top) {
		x_repaint(x);
	}
	usleep(10000);
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	top = false;
	xscreen_t scr;
	x_screen_info(&scr);
	x_t* x = x_open(10, 10, 220, 200, "gtest", X_STYLE_NORMAL);

	int i = 0;
	x->data = &i;
	x->on_repaint = repaint;
	x->on_focus = on_focus;
	x->on_unfocus = on_unfocus;
	x->on_event = event_handle;
	x->on_loop = step;

	x_set_visible(x, true);

	x_run(x);

	x_close(x);
	return 0;
} 
