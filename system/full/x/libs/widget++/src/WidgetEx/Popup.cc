#include <WidgetEx/Popup.h>

using namespace Ewok;

void Popup::onUnfocus() {
    hide();
}

void Popup::hide() {
    setVisible(false); 
}

bool Popup::popup(XWin* owner, int x, int y, uint32_t w, uint32_t h, const char* title, uint32_t style) {
	return open(owner->getX(), owner->getDisplayIndex(), x, y, w, h, title, style, true);
}