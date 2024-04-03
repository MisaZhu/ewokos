#include <WidgetEx/DialogButton.h>
#include <WidgetEx/Dialog.h>

#include <x++/X.h>

using namespace Ewok;

DialogButton::DialogButton(Dialog* dialog, const string& label, int res) : LabelButton(label) {
	this->dialog = dialog;
	result = res;
}

void DialogButton::onClick() {
	dialog->submit(result);
}


