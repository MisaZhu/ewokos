#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <upng/upng.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/klog.h>
#include <ewoksys/proc.h>
#include <x++/X.h>
#include <ttf/ttf.h>

using namespace Ewok;

class TestX : public XWin {
	int fps_counter, fps;
	int fighter_step;
	uint32_t tic;
	graph_t* img_fighter;

	void drawFitgher(graph_t* g) {
		graph_t* img = img_fighter;
		if(img == NULL)
			return;
		graph_blt(img, fighter_step*(img->w/7), 0, img->w/7, img->h,
				g, g->w-img->w/7, g->h-img->h, img->w, img->h);
	}
public:
	inline TestX() {
		tic = 0;
		fps = 0;
		fps_counter = 0;
		fighter_step = 0;
		img_fighter = png_image_new(X::getResName("data/fighter.png"));
	}
	
	inline ~TestX() {
		if(img_fighter != NULL)
			graph_free(img_fighter);
	}

	bool getSize(int32_t& w, int32_t& h) {
		if(img_fighter == NULL)
			return false;
		w = img_fighter->w / 7;
		h = img_fighter->h;
		return true;
	}

protected:
	void onEvent(xevent_t* ev) {
		if(ev->type == XEVT_MOUSE) {
			if(ev->state == XEVT_MOUSE_CLICK)
				this->close();
			else if(ev->state == XEVT_MOUSE_DRAG)
				this->move(ev->value.mouse.rx, ev->value.mouse.ry);
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

		drawFitgher(g);

		fighter_step++;
		if (fighter_step >= 7)
			fighter_step = 0;
	}
};

static void loop(void* p) {
	XWin* xwin = (XWin*)p;
	xwin->repaint();
	proc_usleep(30000);
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	X x;

	int displayNum = x_get_display_num();
	if(displayNum == 0)
		return -1;

	TestX xwin;
	int32_t w, h;
	xwin.getSize(w, h);
	xwin.open(&x, 0, -1, -1, w, h,
			"anim", XWIN_STYLE_NO_FRAME);

	xwin.setDisplay(displayNum-1);
	xwin.setAlpha(true);
	x.run(loop, &xwin);
	return 0;
} 
