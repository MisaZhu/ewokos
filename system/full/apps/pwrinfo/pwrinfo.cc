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

protected:
	void onRepaint(graph_t* g) {
		if(powerFD < 0) {
			powerFD = open("/dev/power0", O_RDONLY);
			if(powerFD < 0)
				return;
		}
		
		uint8_t buf[4];
		if(read(powerFD, buf, 3) != 3)
			return;

		graph_clear(g, 0xffffffff);
		char str[32];
		snprintf(str, 31, "power: %d:%d:%d", buf[0], buf[1], buf[2]);
		graph_draw_text_font(g, 4, 4, str, &font, 0xff000000);
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
	x.open(&desk, &xwin, 100, 100, "pwrInfo", XWIN_STYLE_NO_RESIZE);
	xwin.setVisible(true);

	_xwin = &xwin;
	uint32_t tid = timer_set(1000000, timer_handler);
	x.run(NULL, &xwin);
	timer_remove(tid);

	return 0;
} 
