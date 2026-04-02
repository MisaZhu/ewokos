#include <WidgetEx/FileDialog.h>
#include <Widget/RoundLabelButton.h>
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

static void okFunc(Widget* wd, xevent_t* evt, void* arg) {
	if(evt->type != XEVT_MOUSE || evt->state != MOUSE_STATE_CLICK)
		return;
	FileDialog* dialog = (FileDialog*)wd->getWin();
	dialog->submit(Dialog::RES_OK);
}

static void cancelFunc(Widget* wd, xevent_t* evt, void* arg) {
	if(evt->type != XEVT_MOUSE || evt->state != MOUSE_STATE_CLICK)
		return;
	FileDialog* dialog = (FileDialog*)wd->getWin();
	dialog->submit(Dialog::RES_CANCEL);
}

void FileDialog::onBuild() {
	RootWidget* root = new RootWidget();
	setRoot(root);
	root->setType(Container::VERTICLE);
	

	fileWidget = new FWidget(this);
	root->add(fileWidget);

	Container* c = new Container();
	c->setType(Container::HORIZONTAL);
	c->fix(0, 20);
	root->add(c);

	RoundLabelButton* okButton = new RoundLabelButton("OK");
	okButton->setEventFunc(okFunc);
	c->add(okButton);

	RoundLabelButton* cancelButton = new RoundLabelButton("Cancel");
	cancelButton->setEventFunc(cancelFunc);
	c->add(cancelButton);

}

void FileDialog::setInitPath(const string& path) {
	initPath = path;
	fileWidget->loadDir(initPath);
}

FileDialog::FileDialog(bool path) {
	pathMode = path;
	initPath = "/";
}

string FileDialog::getResult() {
	if(pathMode)
		return fileWidget->getPath();
	return fileWidget->getFile();
}
