#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <vprintf.h>
#include <upng/upng.h>
#include <sys/kernel_tic.h>
#include <sys/klog.h>
#include <x++/X.h>
#include <ttf/ttf.h>

using namespace Ewok;

class TestX : public XWin {
	int fps_counter, fps;
	int fighter_step;
	uint32_t tic;
	graph_t* img_fighter;
	TTY_Font font;
	TTY_Instance inst;

	void drawFitgher(graph_t* g) {
		graph_t* img = img_fighter;
		if(img == NULL)
			return;
		graph_blt(img_fighter, fighter_step*(img->w/7), 0, img->w/7, img->h,
				g, g->w-img->w/7-10, g->h-img->h-10, img->w, img->h);
	}
public:
	inline TestX() {
		tic = 0;
		fps = 0;
		fps_counter = 0;
		fighter_step = 0;
		img_fighter = png_image_new(X::getResName("data/fighter.png"));
    	tty_font_init(&font, "/data/fonts/system.ttf");
    	tty_instance_init(&font, &inst, 32, TTY_INSTANCE_DEFAULT);
	}
	
	inline ~TestX() {
		if(img_fighter != NULL)
			graph_free(img_fighter);
		tty_font_free(&font);
		tty_instance_free(&inst);
	}
protected:
	void onEvent(xevent_t* ev) {
		if(ev->type == XEVT_MOUSE && ev->state == XEVT_MOUSE_UP) {
			this->close();
		}
	}

	void onRepaint(graph_t* g) {
		//int gW = g.getW();
		int gH = g->h;

		fps_counter++;

		uint32_t low;
		kernel_tic32(NULL, NULL, &low); 
		if(tic == 0 || (low - tic) >= 1000000) //1 second
			tic = low;

		graph_clear(g, 0x0);
		if(tic == low) { //1 second
			fps = fps_counter;
			fps_counter = 0;
		}

		char str[32];
		snprintf(str, 31, "EwokOS FPS: %d", fps);
		graph_draw_text_ttf(g, 10, gH-inst.maxGlyphSize.y, str, &font, &inst, 2, 0xffffffff);
		drawFitgher(g);

		fighter_step++;
		if (fighter_step >= 7)
			fighter_step = 0;
	}
};

static void loop(void* p) {
	XWin* xwin = (XWin*)p;
	xwin->repaint();
	usleep(30000);
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	xscreen_t scr;

	X x;

	int displayNum = x_get_display_num();
	if(displayNum == 0)
		return -1;

	x.screenInfo(scr, displayNum-1);

	TestX xwin;
	x.open(&xwin, scr.size.w-160, scr.size.h-160,
			160, 160, "anim", X_STYLE_NO_FRAME | X_STYLE_ALPHA | X_STYLE_NO_FOCUS | X_STYLE_SYSTOP);

	xwin.setDisplay(displayNum-1);
	xwin.setVisible(true);
	x.run(loop, &xwin);
	return 0;
} 
