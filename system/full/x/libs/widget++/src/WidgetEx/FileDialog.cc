#include <WidgetEx/FileDialog.h>
#include <Widget/LabelButton.h>
#include <x++/X.h>

using namespace Ewok;

class FWidget: public FileWidget {
	FileDialog* dialog;
protected:
	void onFile(const string& fname, const string& open_with) {
		dialog->submit();
	}
public: 
	FWidget(FileDialog* d) {
		dialog = d;
	}
};

class OKButton: public LabelButton {
	FileDialog* dialog;
protected:
	void onClick() {
		dialog->submit();
	}
public:
	OKButton(FileDialog* dialog) : LabelButton("OK") {
		this->dialog = dialog;
	}

};

class CancelButton: public LabelButton {
	FileDialog* dialog;
protected:
	void onClick() {
		dialog->cancel();
	}
public:
	CancelButton(FileDialog* dialog) : LabelButton("Cancel") {
		this->dialog = dialog;
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

	OKButton* okButton = new OKButton(this);
	c->add(okButton);
	CancelButton* cancelButton = new CancelButton(this);
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
