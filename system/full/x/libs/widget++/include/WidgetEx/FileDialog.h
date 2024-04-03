#ifndef WIDGET_FILE_DIALOG_HH
#define WIDGET_FILE_DIALOG_HH

#include <WidgetEx/Dialog.h>
#include <WidgetEx/FileWidget.h>

namespace Ewok {

class FileDialog: public Dialog {
protected:
	const string submit();
	FileWidget* fileWidget;
	bool pathMode;

public:
	FileDialog(bool path = false);
};

}

#endif
