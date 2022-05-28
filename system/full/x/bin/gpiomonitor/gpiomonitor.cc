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

extern "C" {
#include <arch/bcm283x/gpio.h>
}

using namespace Ewok;

class GPIOMonitorX : public XWin {
	uint32_t gpioNum;
	int cols;
	font_t*  font;
public:
	inline GPIOMonitorX() {
		font = font_by_name("8x16");
		gpioNum = 40;	
		cols = 10;
	}
protected:
	void onRepaint(graph_t* g) {
		graph_clear(g, 0xffffffff);
		char s[8];

		int rows = gpioNum / cols;
		int w = (g->w-16) / cols;
		int h = (g->h-16) / rows;
		for(int j=0; j<rows; j++) {
			for(int i=0; i<cols; i++) {
				int gpio = j*cols + i;
				snprintf(s, 7, "%d", gpio+1);
				int r = font->h/2;
				int x = i*w + w/2 - r;
				int y = j*h + h/2 - r;
				graph_draw_text(g, x, y, s, font, 0xff000000);
				if(bcm283x_gpio_read(gpio) == 0)
					graph_fill_circle(g, x+r+font->w*2, y+r, r, 0xff00ff00);
				else
					graph_fill_circle(g, x+r+font->w*2, y+r, r, 0xffff0000);
			}
		}
	}
};

static void loop(void* p) {
	XWin* xwin = (XWin*)p;
	xwin->repaint(true);
	usleep(5000);
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	xscreen_t scr;
	X x;
	GPIOMonitorX xwin;

	bcm283x_gpio_init();

	x.screenInfo(scr, 0);
	x.open(&xwin, 60, 40, scr.size.w-120, scr.size.h-80, "GPIOMonitor", X_STYLE_NORMAL);
	xwin.setVisible(true);

	x.run(loop, &xwin);
	return 0;
} 
