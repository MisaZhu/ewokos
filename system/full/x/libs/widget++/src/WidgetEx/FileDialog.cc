#include <WidgetEx/FileDialog.h>
#include <WidgetEx/DialogButton.h>
#include <x++/X.h>

using namespace Ewok;

class FWidget: public FileWidget {
	FileDialog* dialog;
protected:
	void onFile(const string& fname, const string& open_with) {
		dialog->submit(Dialog::RES_OK);
	}
public: 
	FWidget(FileDialog* d) {
		dialog = d;
	}
};

void FileDialog::onOpen() {
	RootWidget* root = new RootWidget();
	setRoot(root);
	root->setType(Container::VERTICLE);
	root->setAlpha(false);

	fileWidget = new FWidget(this);
	root->add(fileWidget);

	Container* c = new Container();
	c->setType(Container::HORIZONTAL);
	c->fix(0, 20);
	root->add(c);

	DialogButton* okButton = new DialogButton(this, "OK", Dialog::RES_OK);
	c->add(okButton);
	DialogButton* cancelButton = new DialogButton(this, "Cancel", Dialog::RES_CANCEL);
	c->add(cancelButton);
}

FileDialog::FileDialog(bool path) {
	pathMode = path;
}

string FileDialog::getResult() {
	if(pathMode)
		return fileWidget->getPath();
	return fileWidget->getFile();
}
