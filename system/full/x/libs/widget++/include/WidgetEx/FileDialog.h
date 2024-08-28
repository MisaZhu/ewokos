#ifndef WIDGET_FILE_DIALOG_HH
#define WIDGET_FILE_DIALOG_HH

#include <WidgetEx/Dialog.h>
#include <WidgetEx/FileWidget.h>

namespace Ewok {

class FileDialog: public Dialog {
protected:
	FileWidget* fileWidget;
	bool pathMode;
	void onBuild();

public:
	FileDialog(bool path = false);
	string getResult();
};

}

#endif
