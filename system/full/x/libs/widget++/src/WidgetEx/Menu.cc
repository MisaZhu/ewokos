#include <WidgetEx/Menu.h>
#include <WidgetEx/Menubar.h>
#include <Widget/RootWidget.h>

using namespace Ewok;

void Menu::onUnfocus() {
    this->setVisible(false);
    menubar->select(-1);
    menubar->getWin()->repaint();
}

Menu::Menu(Menubar* menubar) {
    this->menubar = menubar;
    RootWidget* root = new RootWidget();
	setRoot(root);
	root->setType(Container::VERTICLE);
}