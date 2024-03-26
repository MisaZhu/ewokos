#include <Widget/WidgetWin.h>
#include <Widget/Image.h>
#include <Widget/Label.h>
#include <Widget/LabelButton.h>
#include <Widget/Text.h>
#include <x++/X.h>
#include <unistd.h>
#include <font/font.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/kernel_tic.h>
#include <upng/upng.h>

using namespace Ewok;

class MyButton: public LabelButton {
	uint32_t counter;
protected:
	void onClick() {
		char s[16];
		snprintf(s, 15, "test-%d", counter);
		counter++;
		setLabel(s);
	}
public: 
	MyButton(const string& label = "") : LabelButton(label) {
		counter = 0;
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
	root->setType(Container::VERTICLE);
	root->setAlpha(false);

	Widget* wd = new Anim();
	wd->fix(0, 100);
	root->add(wd);

	Container* c = new Container();
	c->setType(Container::HORIZONTAL);
	c->fix(0, 40);
	root->add(c);

	wd = new MyButton("test");
	c->add(wd);

	wd = new MyButton("disable");
	wd->disable();
	c->add(wd);

	x.open(0, &win, -1, -1, 400, 300, "widgetTest", XWIN_STYLE_NORMAL);
	win.setVisible(true);
	win.setTimer(12);
	x.run(NULL, &win);
	return 0;
}