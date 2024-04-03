#include <WidgetEx/Dialog.h>
#include <x++/X.h>

using namespace Ewok;

Dialog::Dialog() {
	owner = NULL;
}

bool Dialog::popup(XWin* owner, int x, int y, uint32_t w, uint32_t h, const char* title, uint32_t style) {
	this->owner = owner;
	return open(owner->getX(), owner->getDisplayIndex(), x, y, w, h, title, style|XWIN_STYLE_PROMPT, true);
}

void Dialog::cancel() {
	if(owner == NULL)
		return;
	owner->message(this, "");
	close();
}

void Dialog::submit() {
	if(owner == NULL)
		return;
	owner->message(this, getResult());
	close();
}