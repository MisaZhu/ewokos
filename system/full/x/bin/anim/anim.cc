#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <vprintf.h>
#include <upng/upng.h>
#include <sys/basic_math.h>
#include <sys/kernel_tic.h>
#include <sys/klog.h>
#include <x++/X.h>

using namespace Ewok;

class TestX : public XWin {
	int fps_counter, fps;
	int fighter_step;
	uint32_t tic;
	graph_t* img_fighter;
	font_t* font;

	void drawFitgher(Graph& g) {
		graph_t* img = img_fighter;
		if(img == NULL)
			return;
		g.blt(img_fighter, fighter_step*(img->w/7), 0, img->w/7, img->h,
				10, 10, img->w, img->h, 0xff);
	}
public:
	inline TestX() {
		tic = 0;
		fps = 0;
		fps_counter = 0;
		fighter_step = 0;
		img_fighter = png_image_new("/data/images/fighter.png");	
		font = font_by_name("8x16");
	}
	
	inline ~TestX() {
		if(img_fighter != NULL)
			graph_free(img_fighter);
	}
protected:
	void onEvent(xevent_t* ev) {
		if(ev->type == XEVT_MOUSE && ev->state == XEVT_MOUSE_UP) {
			this->close();
		}
	}

	void onRepaint(Graph& g) {
		int gW = g.getW();
		int gH = g.getH();

		fps_counter++;

		uint32_t low;
		kernel_tic32(NULL, NULL, &low); 
		if(tic == 0 || (low - tic) >= 1000000) //1 second
			tic = low;

		g.clear(0x0);
		if(tic == low) { //1 second
			fps = fps_counter;
			fps_counter = 0;
		}

		char str[32];
		snprintf(str, 31, "EwokOS FPS: %d", fps);
		int w;
		get_text_size(str, font, (int32_t*)&w, NULL);
		g.drawText(10, gH-font->h, str, font, 0xffffffff);
		drawFitgher(g);

		int i = fps / 14;
		if(i == 0)
			i = 1;

		if(mod_u32(fps_counter, i) == 0) {
			fighter_step++;
			if (fighter_step >= 7)
				fighter_step = 0;
		}
	}
};

static void loop(void* p) {
	XWin* xwin = (XWin*)p;
	xwin->repaint();
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	xscreen_t scr;

	X x;
	x.screenInfo(scr);

	TestX xwin;
	x.open(&xwin, scr.size.w-160, scr.size.h-160,
			160, 160, "anim", X_STYLE_NO_FRAME | X_STYLE_ALPHA | X_STYLE_NO_FOCUS);

	xwin.setVisible(true);

	x.run(loop, &xwin);
	return 0;
} 
