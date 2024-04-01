#include <Widget/WidgetWin.h>
#include <Widget/Image.h>
#include <Widget/Label.h>
#include <Widget/LabelButton.h>
#include <Widget/List.h>
#include <Widget/EditLine.h>
#include <Widget/Grid.h>
#include <Widget/Scroller.h>
#include <WidgetEx/FileWidget.h>
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

class MyList: public List {
protected:
	void drawItem(graph_t* g, XTheme* theme, int32_t index, const grect_t& r) {
		if(index == itemSelected)
			graph_box(g, r.x, r.y, r.w, r.h, 0xffff0000);
		else
			graph_box(g, r.x, r.y, r.w, r.h, 0xffaaaaaa);
		char s[8];
		snprintf(s, 7, "%d", index);
		graph_draw_text_font(g, r.x+2, r.y+2, s, theme->getFont(), 0xff000000);
	}

	void onSelect(int index) {
		klog("index: %d\n", index);
	}

public:
	MyList() {
	}
};

class Anim: public Widget {
	graph_t* img;
	uint32_t step;
	uint32_t pos;
	int32_t steps;
	EditLine* editLine;
protected:
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		if(img == NULL)
			return;
		graph_gradation(g, r.x, r.y, r.w, r.h, 0xffffffff, 0xff00ff00, true);
		graph_blt_alpha(img, step*(img->w/steps), 0, img->w/steps, img->h,
				g, r.x+pos, r.y+(r.h-img->h)/2, img->w/steps, img->h, 0xff);

		graph_draw_text_font(g, r.x+pos+img->w/steps, r.y+2, editLine->getContent().c_str(), theme->getFont(), theme->basic.fgColor);

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
	Anim(EditLine* editLine) {
		this->editLine = editLine;
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

	EditLine* editLine = new EditLine();
	editLine->fix(0, 30);
	editLine->setContent("hello!");
	root->focus(editLine);
	root->add(editLine);

	Widget* wd = new Anim(editLine);
	wd->fix(0, 100);
	root->add(wd);

	Container* c = new Container();
	c->setType(Container::HORIZONTAL);
	root->add(c);

	wd = new MyButton("test");
	c->add(wd);

	wd = new MyButton("disable");
	wd->disable();
	c->add(wd);

	MyList* list = new MyList();
	c->add(list);
	list->setItemNum(100);
	list->setItemSize(20);

	Scroller *sr = new Scroller();
	sr->fix(8, 0);
	list->setScrollerV(sr);
	c->add(sr);

	WidgetWin win1;
	root = new RootWidget();
	win1.setRoot(root);
	root->setType(Container::VERTICLE);

	list = new MyList();
	root->add(list);
	list->setItemNum(100);
	list->setItemSize(20);
	list->setHorizontal(true);

	sr = new Scroller(true);
	sr->fix(0, 8);
	list->setScrollerH(sr);
	root->add(sr);

	FileWidget* fwd = new FileWidget();
	root->add(fwd);

	x.open(0, &win, -1, -1, 400, 300, "widgetTest", XWIN_STYLE_NORMAL);
	x.open(0, &win1, -1, -1, 400, 300, "subwin", XWIN_STYLE_NORMAL|XWIN_STYLE_PROMPT);
	win.setVisible(true);
	win1.setVisible(true);
	win.setTimer(12);
	x.run(NULL, &win);
	return 0;
}