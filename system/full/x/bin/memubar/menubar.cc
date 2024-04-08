#include <Widget/WidgetWin.h>
#include <WidgetEx/Menubar.h>
#include <WidgetEx/Menu.h>
#include <x++/X.h>
#include <unistd.h>

using namespace Ewok;

int main(int argc, char** argv) {
	X x;
	xscreen_t scr;
	X::getScreenInfo(scr, 0);

	WidgetWin win;
	RootWidget* root = new RootWidget();
	win.setRoot(root);
	root->setType(Container::VERTICLE);

	Menubar* menubar = new Menubar();
	menubar->add("menu1", NULL, new Menu(menubar));
	menubar->add("menu2", NULL, new Menu(menubar));
	root->add(menubar);

	win.open(&x, 0, 0, 0, scr.size.w, 20, "", XWIN_STYLE_SYSBOTTOM | XWIN_STYLE_NO_FRAME);
	x.run(NULL, &win);
	return 0;
}