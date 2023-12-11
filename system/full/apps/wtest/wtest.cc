#include <Widget/WidgetWin.h>
#include <Widget/Image.h>
#include <Widget/Label.h>
#include <x++/X.h>
#include <unistd.h>
#include <font/font.h>
#include <sys/basic_math.h>

using namespace Ewok;

class MyWidget: public Widget {
	font_t font;
	bool down;

protected:
	void onRepaint(graph_t* g, const grect_t& rect) {
		if(down)
			graph_fill(g, rect.x, rect.y, rect.w, rect.h, 0xff222222);
		else
			graph_fill(g, rect.x, rect.y, rect.w, rect.h, 0xffdddddd);

		graph_box(g, rect.x, rect.y, rect.w, rect.h, 0xffffffff);

		if(down)
			graph_draw_text_font_align(g, rect.x, rect.y, rect.w, rect.h,
					"Down!", &font, 0xffdddddd, FONT_ALIGN_CENTER);
		else
			graph_draw_text_font_align(g, rect.x, rect.y, rect.w, rect.h,
					"Widget", &font, 0xff222222, FONT_ALIGN_CENTER);
	}

	bool onMouse(xevent_t* ev) {
		if(ev->state == XEVT_MOUSE_DOWN)
			down = true;
		else if(ev->state == XEVT_MOUSE_UP)
			down = false;
		update();
		return true;
	}

public:
	MyWidget() {
		down = false;
		font_load(DEFAULT_SYSTEM_FONT, 14, &font, true);
	}

	~MyWidget() {
		font_close(&font);
	}
};


/*static void loop(void* p) {
	WidgetWin* xwin = (WidgetWin*)p;
	xwin->repaint();
	usleep(1000000);
}
*/

int main(int argc, char** argv) {
	X x;
	WidgetWin win;

	win.getRootWidget()->setBGColor(0xff000000);

	Widget* wd = new Image("/usr/system/images/mac1984.png");
	wd->setArea(10, 10, 40, 40);
	win.getRootWidget()->add(wd);

	Container* c = new Container();
	c->setType(Container::HORIZONTAL);
	c->setFixed(true);
	c->setArea(0, 0, 40, 40);
	win.getRootWidget()->add(c);

	wd = new Label(X::getSysFont(), "Label");
	c->add(wd);
	wd = new MyWidget();
	c->add(wd);
	wd = new MyWidget();
	wd->resizeTo(100, 0);
	wd->setFixed(true);
	c->add(wd);

	win.getRootWidget()->setType(Container::VERTICLE);

	x.open(&win, 20, 20, 200, 200, "widgetTest", XWIN_STYLE_NORMAL);
	win.setVisible(true);
	//x.run(loop, &win);
	x.run(NULL, &win);
	return 0;
}