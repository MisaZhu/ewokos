#include <WidgetEx/ConfirmDialog.h>
#include <WidgetEx/DialogButton.h>
#include <Widget/Label.h>

#include <x++/X.h>

using namespace Ewok;

void ConfirmDialog::onOpen() {
	RootWidget* root = new RootWidget();
	setRoot(root);
	root->setType(Container::VERTICLE);
	root->setAlpha(false);

	Label* label = new Label(message);
	root->add(label);

	Container* c = new Container();
	c->setType(Container::HORIZONTAL);
	c->fix(0, 20);
	root->add(c);

	DialogButton* okButton = new DialogButton(this, "OK", Dialog::RES_OK);
	c->add(okButton);
	DialogButton* cancelButton = new DialogButton(this, "Cancel", Dialog::RES_CANCEL);
	c->add(cancelButton);
}

ConfirmDialog::ConfirmDialog(const string& msg) {
	setMessage(msg);
}

void ConfirmDialog::setMessage(const string& msg) {
	message = msg;
}
