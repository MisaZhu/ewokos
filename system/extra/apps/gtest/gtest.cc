extern "C" {
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <vprintf.h>
}

#include <x++/X.h>

class TestX : public XWin {
public:
	int i;
protected:
	void onEvent(xevent_t* ev) {
		int key = 0;
		if(ev->type == XEVT_KEYB) {
			key = ev->value.keyboard.value;
			if(key == 27) //esc
				this->close();
		}
	}

	void onRepaint(Graph& g) {
		char str[32];
		font_t* font = font_by_name("12x24");
		snprintf(str, 31, "paint = %d", i++);
		g.clear(argb_int(0xff0000ff));
		g.drawText(10, 10, str, font, 0xffffffff);
		g.fillCircle(100, 100, 50, 0x99000000);
		g.circle(100, 100, 60, 0x99000000);
	}
};

static void loop(void* p) {
	XWin* xwin = (XWin*)p;
	xwin->repaint();
	usleep(10000);
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	xscreen_t scr;
	XWin::screenInfo(scr);

	X x;

	TestX xwin;
	xwin.open(&x, 10, 10, 220, 200, "gtest", X_STYLE_NORMAL | X_STYLE_NO_RESIZE);

	xwin.i = 0;
	xwin.setVisible(true);

	x.run(loop, &xwin);
	return 0;
} 
