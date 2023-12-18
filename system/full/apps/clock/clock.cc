#include <Widget/WidgetWin.h>
#include <Widget/Label.h>
#include <x++/X.h>
#include <unistd.h>
#include <font/font.h>
#include <sys/kernel_tic.h>

using namespace Ewok;

class Timer: public Label {
protected:
	void onTimer() {
		uint32_t ksec;
		kernel_tic(&ksec, NULL);
		uint32_t min = ksec / 60;
		uint32_t hour = min / 60;
		min = min % 60;
		ksec = ksec % 60;
		char s[16];
		snprintf(s, 15, "%02d:%02d:%02d", hour, min, ksec);
		setLabel(s);
	}
public: 
	Timer(const string& label = "00:00:00") : Label(label) {
	}
};

class ClockWin: public WidgetWin {
protected:
	void onRepaint(graph_t* g) {
		setAlpha(true);
		graph_clear(g, 0x88000000);
		WidgetWin::onRepaint(g);
	}

	void onEvent(xevent_t* ev) {
		if(ev->type == XEVT_MOUSE) {
			if(ev->state == XEVT_MOUSE_CLICK)
				this->close();
			else if(ev->state == XEVT_MOUSE_DRAG)
				this->move(ev->value.mouse.rx, ev->value.mouse.ry);
		}
	}
};

int main(int argc, char** argv) {
	X x;
	ClockWin win;

	Theme* theme = new Theme(font_new("/usr/system/fonts/system.ttf", 48, true));
	theme->fgColor = 0xffffffff;
	theme->bgColor = 0xff000000;

	win.setRoot(new RootWidget());
	win.setTheme(theme);
	win.getRoot()->setType(Container::HORIZONTAL);

	Timer* timer = new Timer();
	win.getRoot()->add(timer);

	x.open(0, &win, 200, 50, "clock", XWIN_STYLE_NO_TITLE);
	win.setVisible(true);
	win.setTimer(1);
	x.run(NULL, &win);
	return 0;
}