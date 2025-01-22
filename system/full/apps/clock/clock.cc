#include <Widget/WidgetWin.h>
#include <Widget/Label.h>
#include <x++/X.h>
#include <unistd.h>
#include <font/font.h>
#include <ewoksys/kernel_tic.h>

using namespace Ewok;

class Timer: public Widget {
	uint32_t sec;
	uint32_t min;
	uint32_t hour;

	void drawClock(graph_t* g, XTheme* theme, const grect_t& r, uint32_t clock) {
		graph_gradation(g, r.x, r.y, r.w, r.h/2, 0xffffffff, 0xffaaaaaa, true);
		graph_gradation(g, r.x, r.y+r.h/2, r.w, r.h/2, 0xffffffff, 0xffaaaaaa, true);

		char txt[4];
		snprintf(txt, 3, "%02d", clock);
		graph_draw_text_font_align(g, r.x, r.y, r.w, r.h,
				txt, theme->getFont(), theme->basic.fontSize, theme->basic.fgColor, FONT_ALIGN_CENTER);

		graph_set(g, r.x, r.y+r.h/2-1, r.w, 2, 0xaa000000);
		graph_box(g, r.x, r.y, r.w, r.h/2-1, 0xff000000);
		graph_box(g, r.x, r.y+r.h/2+1, r.w, r.h/2-1, 0xff000000);
	}

protected:

	void onRepaint(graph_t* g, XTheme* theme, const grect_t& rect) {
		graph_set(g, rect.x, rect.y+8, rect.w, rect.h-16, 0xaa000000);
		uint32_t w = (rect.w-8)/3;
		grect_t r = {rect.x+4, rect.y, w-2, rect.h};
		drawClock(g, theme, r, hour);
		r.x += (w+1);
		drawClock(g, theme, r, min);
		r.x += (w+1);
		drawClock(g, theme, r, sec);
	}

	void onTimer(uint32_t timerFPS, uint32_t timerStep) {
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
public:
	ClockWin() {
		theme.setFont("system", 48);
		theme.basic.bgColor = 0xffffffff;
		theme.basic.fgColor = 0xff000000;
	}
};

int main(int argc, char** argv) {
	X x;
	ClockWin win;


	RootWidget* root = new RootWidget();
	win.setRoot(root);
	root->setType(Container::HORIZONTAL);

	Timer* timer = new Timer();
	root->add(timer);

	win.open(&x, 0, -1, -1, 200, 68, "clock", XWIN_STYLE_NO_FRAME);
	win.setTimer(1);
	x.run(NULL, &win);
	return 0;
}