#include <Widget/WidgetWin.h>
#include <x++/X.h>
#include <unistd.h>
#include <ttf/ttf.h>
#include <sys/basic_math.h>

using namespace Ewok;

class MyWidget: public Widget {
	ttf_font_t* font;

protected:
	void onRepaint(graph_t* g, grect_t* rect) {
		graph_fill(g, rect->x, rect->y, rect->w, rect->h, 0xffff0000);
		graph_box(g, rect->x, rect->y, rect->w, rect->h, 0xffffffff);
		graph_draw_text_ttf(g, rect->x, rect->y, "widget", font, 0xff0000ff);
	}
public:
	MyWidget() {
		font = ttf_font_load("/data/fonts/system.ttf", 14);
	}
};


static void loop(void* p) {
	WidgetWin* xwin = (WidgetWin*)p;
	xwin->repaint();
	usleep(1000000);
}

int main(int argc, char** argv) {
	X x;
	WidgetWin win;

	Widget* wd = new MyWidget();
	wd->setRect(10, 10, 40, 40);
	win.getWidget()->add(wd);

	Container* c = new Container();
	c->setType(Container::HORIZONTAL);
	c->setFixed(true);
	c->setRect(0, 0, 40, 40);
	win.getWidget()->add(c);

	wd = new MyWidget();
	c->add(wd);
	wd = new MyWidget();
	c->add(wd);
	wd = new MyWidget();
	wd->resizeTo(100, 0);
	wd->setFixed(true);
	c->add(wd);

	win.getWidget()->setType(Container::VERTICLE);

	x.open(&win, 20, 20, 200, 200, "widgetTest", X_STYLE_NORMAL);
	win.setVisible(true);
	x.run(loop, &win);
	return 0;
}