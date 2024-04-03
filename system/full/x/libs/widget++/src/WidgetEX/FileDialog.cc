#include <WidgetEx/FileDialog.h>
#include <x++/X.h>

using namespace Ewok;

class FWidget: public FileWidget {
	FileDialog* dialog;
protected:
	void onFile(const string& fname, const string& open_with) {
		dialog->close();
	}
public: 
	FWidget(FileDialog* d) {
		dialog = d;
	}
};

FileDialog::FileDialog(bool path) {
	RootWidget* root = new RootWidget();
	setRoot(root);
	root->setType(Container::VERTICLE);
	root->setAlpha(false);

	fileWidget = new FWidget(this);
	root->add(fileWidget);
	pathMode = path;
}

const string FileDialog::submit() {
	if(pathMode)
		return fileWidget->getPath();
	return fileWidget->getFile();
}
