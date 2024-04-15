#include <WidgetEx/Popup.h>

using namespace Ewok;

void Popup::onUnfocus() {
    hide();
}

void Popup::hide() {
    setVisible(false); 
}

void Popup::onOpen() {
	RootWidget* root = new RootWidget();
	setRoot(root);
	root->setType(Container::VERTICLE);
	root->setAlpha(false);
}

bool Popup::popup(XWin* owner, int x, int y, uint32_t w, uint32_t h, const char* title, uint32_t style) {
	return open(owner->getX(), owner->getDisplayIndex(), x, y, w, h, title, style, true);
}