#include <Widget/WidgetWin.h>
#include <Widget/Image.h>
#include <Widget/Label.h>
#include <Widget/Button.h>
#include <x++/X.h>
#include <unistd.h>
#include <font/font.h>
#include <sys/basic_math.h>

using namespace Ewok;

class MyButton: public Button {
protected:
	void paintDown(graph_t* g, const Theme* theme, const grect_t& r) {
		Button::paintDown(g, theme, r);
		graph_draw_text_font_align(g, r.x, r.y, r.w, r.h,
				"Down!", theme->font, theme->fgColor, FONT_ALIGN_CENTER);
	}

	void paintUp(graph_t* g, const Theme* theme, const grect_t& r) {
		Button::paintUp(g, theme, r);
		graph_draw_text_font_align(g, r.x, r.y, r.w, r.h,
					"Widget", theme->font, theme->fgColor, FONT_ALIGN_CENTER);
	}

	void paintDisabled(graph_t* g, const Theme* theme, const grect_t& r) {
		Button::paintDisabled(g, theme, r);
		graph_draw_text_font_align(g, r.x, r.y, r.w, r.h,
					"Disable", theme->font, 0xffdddddd, FONT_ALIGN_CENTER);
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
	win.setRoot(new RootWidget());

	Widget* wd = new Image("/usr/system/images/mac1984.png");
	win.getRoot()->add(wd);

	Container* c = new Container();
	c->setType(Container::HORIZONTAL);
	c->fix(0, 40);
	win.getRoot()->add(c);

	wd = new Label(X::getSysFont(), "Label");
	c->add(wd);

	wd = new MyButton();
	c->add(wd);

	wd = new MyButton();
	wd->fix(100, 0);
	wd->disable();
	c->add(wd);

	win.getRoot()->setType(Container::VERTICLE);

	x.open(0, &win, 400, 300, "widgetTest", XWIN_STYLE_NORMAL);
	win.setVisible(true);
	//x.run(loop, &win);
	x.run(NULL, &win);
	return 0;
}