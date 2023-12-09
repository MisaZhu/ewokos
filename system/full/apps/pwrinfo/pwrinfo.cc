#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <vprintf.h>
#include <upng/upng.h>
#include <sys/basic_math.h>
#include <sys/kernel_tic.h>
#include <font/font.h>
#include <x++/X.h>
#include <sys/timer.h>

using namespace Ewok;

class PowerInfoX : public XWin {
	font_t font;
	int powerFD;

	void drawNoBat(graph_t* g, const grect_t& r) {
		graph_fill(g, r.x, r.y, r.w, r.h, 0xffdddddd);
		graph_box(g, r.x, r.y, r.w, r.h, 0xff000000);
	}

	void drawCharging(graph_t* g, const grect_t& r, int bat) {
		static bool b = true;
		int w = r.w*bat/100;
		if(b)
			graph_fill(g, r.x+r.w-w, r.y, w, r.h, 0xffdddddd);
		else
			graph_fill(g, r.x+r.w-w, r.y, w, r.h, 0xff22dd22);

		graph_box(g, r.x, r.y, r.w, r.h, 0xff000000);
		b = !b;
	}

	void drawBat(graph_t* g, const grect_t& r, int bat) {
		int w = r.w*bat/100;
		graph_fill(g, r.x+r.w-w, r.y, w, r.h, 0xff22dd22);
		graph_box(g, r.x, r.y, r.w, r.h, 0xff000000);
	}

protected:
	void onRepaint(graph_t* g) {
		setAlpha(true);
		graph_clear(g, 0x0);
		grect_t r = {4, 4, g->w-8, g->h-8};
		graph_box(g, r.x, r.y+2, 5, r.h-4, 0xff000000);
		r.x += 4;
		r.w -= 4;

		if(powerFD < 0)
			powerFD = open("/dev/power0", O_RDONLY);

		uint8_t buf[4];
		if(powerFD < 0){
			drawNoBat(g, r);
			return;
		}

		if(read(powerFD, buf, 3) == 3){
			if(!buf[0])
				drawNoBat(g, r);
			else if(buf[1])
				drawCharging(g, r, buf[2]);
			else
				drawBat(g, r, buf[2]);
		}
		else {
			drawNoBat(g, r);
		}
	}

public:
	inline PowerInfoX() {
		powerFD = -1;
		font_load(DEFAULT_SYSTEM_FONT, 13, &font, true);
	}
	
	inline ~PowerInfoX() {
		if(font.id >= 0)
			font_close(&font);
		if(powerFD > 0)
			::close(powerFD);
	}
};

static XWin* _xwin = NULL;
static void timer_handler(void) {
	_xwin->repaint();
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	grect_t desk;

	X x;
	x.getDesktopSpace(desk, 0);

	PowerInfoX xwin;
	x.open(&desk, &xwin, 100, 32, "pwrInfo", XWIN_STYLE_NO_RESIZE);
	xwin.setVisible(true);

	_xwin = &xwin;
	uint32_t tid = timer_set(1000000, timer_handler);
	x.run(NULL, &xwin);
	timer_remove(tid);

	return 0;
} 
