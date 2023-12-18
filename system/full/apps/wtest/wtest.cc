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
#include <sys/klog.h>

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

class MyLabel: public Label {
	uint32_t counter;
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
		counter++;
	}
public: 
	MyLabel(const string& label = "") : Label(label) {
		counter = 0;
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
	win.getRoot()->setAlpha(false);

	Container* c = new Container();
	c->setType(Container::HORIZONTAL);
	win.getRoot()->add(c);

	Widget* wd = new Image("/usr/system/images/mac1984.png");
	c->add(wd);

	Text* txt = new Text("text\nHello world\n[中文测试]\n123～！@");
	Theme* theme = new Theme(font_new("/usr/system/fonts/system_cn.ttf", 18, true));
	theme->bgColor = 0xff000000;
	theme->fgColor = 0xffffaa88;
	txt->setTheme(theme);
	c->add(txt);

	c = new Container();
	c->setType(Container::HORIZONTAL);
	c->fix(0, 40);
	win.getRoot()->add(c);

	MyLabel* label = new MyLabel("Label");
	theme = new Theme(font_new("/usr/system/fonts/system.ttf", 18, true));
	label->setTheme(theme);
	label->setAlpha(false);
	c->add(label);

	wd = new MyButton("test");
	c->add(wd);

	wd = new MyButton("disable");
	wd->disable();
	c->add(wd);

	win.getRoot()->setType(Container::VERTICLE);

	x.open(0, &win, 400, 300, "widgetTest", XWIN_STYLE_NORMAL);
	win.setVisible(true);
	win.setTimer(60);
	//x.run(loop, &win);
	x.run(NULL, &win);
	return 0;
}