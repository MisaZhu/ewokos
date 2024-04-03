#ifndef WIDGET_DIALOG_HH
#define WIDGET_DIALOG_HH

#include <Widget/WidgetWin.h>
#include <string>

using namespace EwokSTL;
namespace Ewok {

class Dialog: public WidgetWin {
protected:
	XWin* owner;
	virtual const string submit() = 0;
	bool onClose();
public:
	Dialog();
	bool popup(XWin* owner, int x, int y, uint32_t w, uint32_t h, const char* title, uint32_t style);
};

}

#endif
