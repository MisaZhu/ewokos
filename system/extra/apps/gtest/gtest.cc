extern "C" {
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <vprintf.h>
}

#include <x++/X.h>

class TestX : public X {
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

	void onRepaint(graph_t* g) {
		char str[32];
		font_t* font = font_by_name("12x24");
		snprintf(str, 31, "paint = %d", i++);
		clear(g, argb_int(0xff0000ff));
		draw_text(g, 10, 10, str, font, 0xffffffff);
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
	X::screenInfo(&scr);

	TestX* x = new TestX();
	x->open(10, 10, 220, 200, "gtest", X_STYLE_NORMAL | X_STYLE_NO_RESIZE);

	x->i = 0;
	x->setVisible(true);
	x->run();
	delete x;
	return 0;
} 
