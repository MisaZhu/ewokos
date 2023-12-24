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
	uint32_t pos;
	int32_t steps;
protected:
	void onRepaint(graph_t* g, const Theme* theme, const grect_t& r) {
		if(img == NULL)
			return;
		//graph_fill(g, r.x, r.y, r.w, r.h, 0xff00ff00);
		graph_gradation(g, r.x, r.y, r.w, r.h, 0xffffffff, 0xff00ff00, true);
		graph_blt_alpha(img, step*(img->w/steps), 0, img->w/steps, img->h,
				//g, r.x+(r.w-img->w/steps)/2, r.y+(r.h-img->h)/2, img->w/steps, img->h, 0xff);
				g, r.x+pos, r.y+(r.h-img->h)/2, img->w/steps, img->h, 0xff);

		if(pos > r.w)
			pos = 0;
	}

	void onTimer() {
		step++;
		if (step >= steps)
			step = 0;
		pos += img->w/steps/steps;
		update();
	}

public: 
	Anim() {
		step = 0;
		img = png_image_new(X::getResName("data/walk.png"));
		steps = 8;
		pos = 0;
	}

	~Anim() {
		if(img != NULL)
			graph_free(img);
	}
};

int main(int argc, char** argv) {
	X x;
	WidgetWin win;
	RootWidget* root = new RootWidget();
	win.setRoot(root);
	root->setAlpha(false);

	Widget* wd = new Anim();
	wd->fix(0, 100);
	root->add(wd);

	Text* txt = new Text("text\nHello world\n[中文测试]\n123～！@");
	Theme* theme = new Theme(font_new("/usr/system/fonts/system_cn.ttf", 18, true));
	theme->bgColor = 0xff000000;
	theme->fgColor = 0xffffaa88;
	txt->setTheme(theme);
	root->add(txt);

	Container* c = new Container();
	c->setType(Container::HORIZONTAL);
	c->fix(0, 40);
	root->add(c);

	wd = new MyButton("test");
	c->add(wd);

	wd = new MyButton("disable");
	wd->disable();
	c->add(wd);

	root->setType(Container::VERTICLE);

	x.open(0, &win, 400, 300, "widgetTest", XWIN_STYLE_NORMAL);
	win.setVisible(true);
	win.setTimer(12);
	x.run(NULL, &win);
	return 0;
}