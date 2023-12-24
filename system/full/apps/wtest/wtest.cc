#include <Widget/WidgetWin.h>
#include <Widget/Image.h>
#include <Widget/Label.h>
#include <Widget/LabelButton.h>
#include <Widget/Text.h>
#include <x++/X.h>
#include <unistd.h>
#include <font/font.h>
#include <sys/basic_math.h>
#include <sys/kernel_tic.h>
#include <upng/upng.h>

using namespace Ewok;

class MyButton: public LabelButton {
protected:
	void onClick() {
		klog("click %s!\n", label.c_str());
	}
public: 
	MyButton(const string& label = "") : LabelButton(label) {
	}
};

class Anim: public Widget {
	graph_t* img;
	uint32_t step;
	int32_t steps;
protected:
	void onRepaint(graph_t* g, const Theme* theme, const grect_t& r) {
		if(img == NULL)
			return;
		graph_fill(g, r.x, r.y, r.w, r.h, theme->bgColor);
		graph_blt_alpha(img, step*(img->w/steps), 0, img->w/steps, img->h,
				g, r.x+(r.w-img->w/steps)/2, r.y+(r.h-img->h)/2, img->w/steps, img->h, 0xff);
	}

	void onTimer() {
		step++;
		if (step >= steps)
			step = 0;
		update();
	}

public: 
	Anim() {
		step = 0;
		img = png_image_new(X::getResName("data/walk.png"));
		steps = 8;
	}

	~Anim() {
		if(img != NULL)
			graph_free(img);
	}
};

int main(int argc, char** argv) {
	X x;
	WidgetWin win;
	win.setRoot(new RootWidget());
	win.getRoot()->setAlpha(false);

	Container* c = new Container();
	c->setType(Container::HORIZONTAL);
	win.getRoot()->add(c);

	Widget* wd = new Image("/usr/system/icons/ewok.png");
	c->add(wd);

	wd = new Anim();
	c->add(wd);

	Text* txt = new Text("text\nHello world\n[中文测试]\n123～！@");
	Theme* theme = new Theme(font_new("/usr/system/fonts/system_cn.ttf", 18, true));
	theme->bgColor = 0xff000000;
	theme->fgColor = 0xffffaa88;
	txt->setTheme(theme);
	win.getRoot()->add(txt);

	c = new Container();
	c->setType(Container::HORIZONTAL);
	c->fix(0, 40);
	win.getRoot()->add(c);

	wd = new MyButton("test");
	c->add(wd);

	wd = new MyButton("disable");
	wd->disable();
	c->add(wd);

	win.getRoot()->setType(Container::VERTICLE);

	x.open(0, &win, 400, 300, "widgetTest", XWIN_STYLE_NORMAL);
	win.setVisible(true);
	win.setTimer(10);
	x.run(NULL, &win);
	return 0;
}