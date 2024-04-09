#include <Widget/WidgetWin.h>
#include <WidgetEx/Menubar.h>
#include <WidgetEx/Menu.h>
#include <Widget/LabelButton.h>
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

	Menu* submenu = new Menu();
	submenu->add("submenu1", NULL, NULL, NULL, NULL);
	submenu->add("submenu2", NULL, NULL, NULL, NULL);
	submenu->add("submenu3", NULL, NULL, NULL, NULL);


	Menu* submenu1 = new Menu();
	submenu1->add("submenu1", NULL, NULL, NULL, NULL);
	submenu1->add("submenu2", NULL, submenu, NULL, NULL);
	submenu1->add("submenu3", NULL, NULL, NULL, NULL);

	Menu* menu = new Menu();
	menu->add("submenu1", NULL, NULL, NULL, NULL);
	menu->add("submenu2", NULL, NULL, NULL, NULL);
	menu->add("submenu3", NULL, submenu1, NULL, NULL);


	Menubar* menubar = new Menubar();
	menubar->add("menu1", NULL, menu, NULL, NULL);
	menubar->fix(0, 20);
	root->add(menubar);

	win.open(&x, 0, 0, 0, scr.size.w, 20, "", XWIN_STYLE_SYSBOTTOM | XWIN_STYLE_NO_FRAME);
	x.run(NULL, &win);
	return 0;
}