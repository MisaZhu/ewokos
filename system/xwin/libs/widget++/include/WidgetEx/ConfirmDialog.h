#ifndef WIDGET_CONFIRM_DIALOG_HH
#define WIDGET_CONFIRM_DIALOG_HH

#include <WidgetEx/Dialog.h>

#include <string>
using namespace std;

namespace Ewok {

#define  CONFIRM_DIALOG_RESULT  "dialog.ok"

class ConfirmDialog: public Dialog {
protected:
	string message;
	void onBuild();
public:
	ConfirmDialog(const string& msg = "");
	void setMessage(const string& msg);
};

}

#endif
