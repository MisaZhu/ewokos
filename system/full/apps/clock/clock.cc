#include <Widget/WidgetWin.h>
#include <Widget/Label.h>
#include <x++/X.h>
#include <unistd.h>
#include <font/font.h>
#include <sys/kernel_tic.h>

using namespace Ewok;

class Timer: public Widget {
	uint32_t sec;
	uint32_t min;
	uint32_t hour;

	void drawClock(graph_t* g, const Theme* theme, const grect_t& r, uint32_t clock) {
		graph_gradation(g, r.x, r.y, r.w, r.h/2, 0xffffffff, 0xffaaaaaa, true);
		graph_gradation(g, r.x, r.y+r.h/2, r.w, r.h/2, 0xffffffff, 0xffaaaaaa, true);

		char txt[4];
		snprintf(txt, 3, "%02d", clock);
		graph_draw_text_font_align(g, r.x, r.y, r.w, r.h,
				txt, theme->font, theme->fgColor, FONT_ALIGN_CENTER);

		graph_set(g, r.x, r.y+r.h/2-1, r.w, 2, 0x0);
		graph_box(g, r.x, r.y, r.w, r.h/2-1, 0xff000000);
		graph_box(g, r.x, r.y+r.h/2+1, r.w, r.h/2-1, 0xff000000);
	}

protected:

	void onRepaint(graph_t* g, const Theme* theme, const grect_t& rect) {
		grect_t r = {rect.x, rect.y, rect.w/3-2, rect.h};
		drawClock(g, theme, r, hour);
		r.x += (rect.w/3+1);
		drawClock(g, theme, r, min);
		r.x += (rect.w/3+1);
		drawClock(g, theme, r, sec);
	}

	void onTimer() {
		uint32_t ksec;
		kernel_tic(&ksec, NULL);
		min = ksec / 60;
		hour = min / 60;
		min = min % 60;
		sec = ksec % 60;
		update();
	}
public: 
	Timer() {
		sec = 0;
		min = 0;
		hour = 0;
	}
};

class ClockWin: public WidgetWin {
protected:
	void onRepaint(graph_t* g) {
		setAlpha(true);
		graph_clear(g, 0x0);
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
	theme->bgColor = 0xffffffff;
	theme->fgColor = 0xff000000;

	RootWidget* root = new RootWidget();
	win.setRoot(root);
	win.setTheme(theme);
	root->setType(Container::HORIZONTAL);

	Timer* timer = new Timer();
	root->add(timer);

	x.open(0, &win, 200, 68, "clock", XWIN_STYLE_NO_FRAME);
	win.setVisible(true);
	win.setTimer(1);
	x.run(NULL, &win);
	return 0;
}