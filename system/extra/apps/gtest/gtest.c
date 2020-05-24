#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <vprintf.h>
#include <x/xclient.h>

static bool top = false;
void on_focus(x_t* x, void* p) {
	(void)x;
	(void)p;
	top = true;
}

void on_unfocus(x_t* x, void* p) {
	(void)x;
	(void)p;
	top = false;
}

void event_handle(x_t* x, xevent_t* ev, void* p) {
	(void)p;
	int key = 0;
	if(ev->type == XEVT_KEYB) {
		key = ev->value.keyboard.value;
		if(key == 27) //esc
			x->closed = true;
	}
}

void step(x_t* x, void* p) {
	int* i = (int*)p;

	if(top) {
		char str[32];
		font_t* font = font_by_name("12x24");
		snprintf(str, 31, "paint = %d", (*i)++);
		graph_t* g = x_get_graph(x);
		clear(g, argb_int(0xff0000ff));
		draw_text(g, 10, 10, str, font, 0xffffffff);
		x_release_graph(x, g);
		x_update(x);
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
	x->on_focus = on_focus;
	x->on_unfocus = on_unfocus;

	x_set_visible(x, true);

	int i = 0;
	x_run(x, event_handle, step, &i);

	x_close(x);
	return 0;
} 
