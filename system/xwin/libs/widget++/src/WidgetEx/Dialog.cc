#include <WidgetEx/Dialog.h>
#include <x++/X.h>

using namespace Ewok;

Dialog::Dialog() {
	owner = NULL;
	arg = NULL;
}

bool Dialog::popup(XWin* owner, uint32_t w, uint32_t h, const char* title, uint32_t style, void* arg) {
	this->owner = owner;
	this->arg = arg;

	xscreen_info_t scr;
	X::getScreenInfo(scr, owner->getDisplayIndex());

	if(w == 0)
		w = scr.size.w*2/3;
	if(h == 0)
		h = scr.size.h*2/3;

	int x = (scr.size.w - (int)w)/2;
	int y = (scr.size.h - (int)h)/2;

	bool ret = open(owner->getX(), owner->getDisplayIndex(), x, y, w, h, title, style|XWIN_STYLE_PROMPT, false);
	build();
	setVisible(true);
	return ret;
}

void Dialog::submit(int res) {
	if(owner == NULL)
		return;
	owner->dialoged(this, res, arg);
	close();
}