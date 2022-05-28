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
	static const int margin = 8;
public:
	inline GPIOMonitorX() {
		font = font_by_name("8x16");
		gpioNum = 40;	
		cols = 10;
	}

	void drawGPIO(graph_t* g, int x, int y, int w, int h, int gpio) {
		graph_box(g, x, y, w, h, 0xff000000);

		int r = font->h/2;
		x += 4;
		y += h/2 - r;

		char s[8];
		snprintf(s, 7, "%d", gpio);
		graph_draw_text(g, x, y, s, font, 0xff000000);

		int v = bcm283x_gpio_read(gpio);
		if(v == 0)
			graph_fill_circle(g, x+r+font->w*2, y+r, r, 0xff00ff00);
		else
			graph_fill_circle(g, x+r+font->w*2, y+r, r, 0xffff0000);

		snprintf(s, 7, "%d", v);
		graph_draw_text(g, x+font->w*2+font->w/2, y, s, font, 0xff000000);

		snprintf(s, 7, "%d", gpio);
		graph_draw_text(g, x, y, s, font, 0xff000000);
	}

protected:
	void onRepaint(graph_t* g) {
		graph_clear(g, 0xffffffff);

		int rows = gpioNum / cols;
		int w = (g->w - margin*2) / cols;
		int h = (g->h - margin*2) / rows;
		for(int j=0; j<rows; j++) {
			for(int i=0; i<cols; i++) {
				int gpio = j*cols + i;
				int x = i*w + margin;
				int y = j*h + margin;
				drawGPIO(g, x, y, w, h, gpio);
			}
		}
	}

	void onEvent(xevent_t* ev) {
		xinfo_t xinfo;
		getInfo(xinfo);
		int rows = gpioNum / cols;
		int w = (xinfo.wsr.w - margin*2) / cols;
		int h = (xinfo.wsr.h - margin*2) / rows;

		if(ev->type == XEVT_MOUSE && ev->state == XEVT_MOUSE_UP) {
			int col = (ev->value.mouse.winx - margin) / w;
			int row = (ev->value.mouse.winy - margin) / h;
			int gpio = row*cols + col;
			if(gpio >= gpioNum)
				return;

			uint32_t v = bcm283x_gpio_read(gpio);
			v = (v == 0) ?  1 : 0;
			bcm283x_gpio_config(gpio, GPIO_OUTPUT);
			bcm283x_gpio_write(gpio, v);
			repaint(true);
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
