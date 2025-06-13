#include <WidgetEx/Dialog.h>
#include <x++/X.h>

using namespace Ewok;

Dialog::Dialog() {
	owner = NULL;
}

bool Dialog::popup(XWin* owner, uint32_t w, uint32_t h, const char* title, uint32_t style) {
	this->owner = owner;

	xscreen_info_t scr;
	X::getScreenInfo(scr, owner->getDisplayIndex());

	if(w == 0)
		w = scr.size.w/3;
	if(h == 0)
		h = scr.size.h/3;

	int x = (scr.size.w - w)/2;
	int y = (scr.size.h - h)/2;
	bool ret = open(owner->getX(), owner->getDisplayIndex(), x, y, w, h, title, style|XWIN_STYLE_PROMPT, false);
	build();
	setVisible(true);
	return ret;
}

void Dialog::submit(int res) {
	if(owner == NULL)
		return;
	owner->dialoged(this, res);
	close();
}