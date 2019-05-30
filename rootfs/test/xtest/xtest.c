#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <kstring.h>
#include <vprintf.h>
#include <x/xclient.h>
#include <graph/tga.h>

int main(int argc, char* argv[]) {
	int pos_x=100, pos_y=100;
	if(argc == 3) {
		pos_x = atoi(argv[1]);
		pos_y = atoi(argv[2]);
	}
	printf("pos: %d, %d\n", pos_x, pos_y);

	const char* dev_name = "/dev/X0";

	x_t x;
	if(x_open(dev_name, &x) != 0)
		return -1;
	
	x_set_title(&x, argv[0]);
	font_t* font = get_font_by_name("8x10");
	x_move_to(&x, pos_x, pos_y);
	x_resize_to(&x, 400, 300);
	x_set_state(&x, X_STATE_NORMAL);
	graph_t* g = x_get_graph(&x);
	clear(g, 0x0);
	x_flush(&x);

	printf("loading...\n");
	tga_image* tga = tga_image_new("/data/res/16.tga");
	graph_t* img = graph_new((uint32_t*)tga_image_get_data(tga),
										tga_image_get_width(tga),
										tga_image_get_height(tga));
	printf("ok.\n");

	while(true) {
		x_ev_t ev;
		if(x_get_event(dev_name, &ev) == 0) {
			char s[32] = "test";
			if(ev.type == X_EV_KEYB) {
				snprintf(s, 31, "key event: %d", ev.value.keyboard.value);
			}
			else if(ev.type == X_EV_MOUSE) {
				snprintf(s, 31, "mouse event: %d, %d", ev.value.mouse.x, ev.value.mouse.y);
			}
			else if(ev.type == X_EV_WIN) {
				if(ev.state == X_EV_WIN_CLOSE)
					break;
			}

			clear(g, 0xff000000);
			blt_alpha(img, 0, 0, img->w, img->h,
					g, 0, 100, img->w, img->h, 0xff);
			draw_text(g, 10, 10, s, font, 0xffffffff);
			x_flush(&x);
		}
	}

	tga_image_destroy(tga);
	graph_free(img);
	graph_free(g);
	x_close(&x);
	return 0;
}

