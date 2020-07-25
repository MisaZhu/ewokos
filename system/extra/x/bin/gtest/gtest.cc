#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <vprintf.h>
#include <sys/basic_math.h>
#include <x++/X.h>

using namespace Ewok;

class TestX : public XWin {
	int count;
	bool circle;
public:
	inline TestX() {
		count = 0;
		circle = true;
	}
protected:
	void onEvent(xevent_t* ev) {
		int key = 0;
		if(ev->type == XEVT_IM) {
			key = ev->value.im.value;
			if(key == 27) //esc
				this->close();
		}
	}

	void onRepaint(Graph& g) {
		char str[32];
		font_t* font = font_by_name("16x32");

		int x = random_to(g.getW());
		int y = random_to(g.getH());
		int w = random_to(128);
		int h = random_to(128);
		int c = random();

		if(circle) {
			g.fillCircle(x, y, w, c);
			g.circle(x, y, w+10, c);
		}
		else {
			g.fill(x+5, y+5, w-10, h-10, c);
			g.box(x, y, w, h, c);
		}

		snprintf(str, 31, "paint = %d", count++);
		g.fill(0, 0, g.getW(), font->h, 0xff000000);
		g.drawText(0, 0, str, font, 0xffffffff);

		circle = !circle;	
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

	X x;
	x.screenInfo(scr);

	TestX xwin;
	x.open(&xwin, 10, 10, scr.size.w/2-20, scr.size.h/2-20, "gtest", X_STYLE_NORMAL);

	xwin.setVisible(true);

	x.run(loop, &xwin);
	return 0;
} 
