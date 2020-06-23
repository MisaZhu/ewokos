extern "C" {
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <vprintf.h>
#include <sys/basic_math.h>
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

		int x = mod_u32(random() >> 16, g.getW());
		int y = mod_u32(random() >> 16, g.getH());
		int w = mod_u32(random() >> 16, 256);
		int h = mod_u32(random() >> 16, 256);
		int c = random();

		g.fill(x+5, y+5, w-10, h-10, c);
		g.box(x, y, w, h, c);

		snprintf(str, 31, "paint = %d", i++);
		g.fill(0, 0, g.getW(), font->h, 0xff000000);
		g.drawText(0, 0, str, font, 0xffffffff);
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
	x.open(&xwin, 10, 10, scr.size.w-20, scr.size.h-20, "gtest", X_STYLE_NORMAL);

	xwin.i = 0;
	xwin.setVisible(true);

	x.run(loop, &xwin);
	return 0;
} 
