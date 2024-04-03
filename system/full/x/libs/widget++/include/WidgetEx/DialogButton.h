#ifndef WIDGET_DIALOG_BUTTON_HH
#define WIDGET_DIALOG_BUTTON_HH

#include <Widget/LabelButton.h>

#include <string>
using namespace EwokSTL;

namespace Ewok {

class Dialog;

class DialogButton: public LabelButton {
protected:
	Dialog* dialog;
	int result;
	void onClick();
public:
	DialogButton(Dialog* dialog, const string& label, int res);
};


}

#endif
