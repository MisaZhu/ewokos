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
		g.fillCircle(40, 40, 50, 0x99000000);
	}

	void onLoop(void) {
		repaint();
		usleep(10000);
	}
};

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	xscreen_t scr;
	XWin::screenInfo(scr);

	TestX* x = new TestX();
	x->open(10, 10, 220, 200, "gtest", X_STYLE_NORMAL | X_STYLE_NO_RESIZE);

	x->i = 0;
	x->setVisible(true);
	x->run();
	delete x;
	return 0;
} 
