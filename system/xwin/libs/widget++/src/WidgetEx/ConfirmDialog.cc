#include <WidgetEx/ConfirmDialog.h>
#include <Widget/RoundLabelButton.h>
#include <Widget/Label.h>

#include <x++/X.h>

using namespace Ewok;

static void okFunc(Widget* wd, xevent_t* evt, void* arg) {
	if(evt->type != XEVT_MOUSE || evt->state != MOUSE_STATE_CLICK)
		return;
	ConfirmDialog* dialog = (ConfirmDialog*)wd->getWin();
	dialog->submit(Dialog::RES_OK);
}

static void cancelFunc(Widget* wd, xevent_t* evt, void* arg) {
	if(evt->type != XEVT_MOUSE || evt->state != MOUSE_STATE_CLICK)
		return;
	ConfirmDialog* dialog = (ConfirmDialog*)wd->getWin();
	dialog->submit(Dialog::RES_CANCEL);
}

void ConfirmDialog::onBuild() {
	RootWidget* root = new RootWidget();
	setRoot(root);
	root->setType(Container::VERTICAL);
	root->setAlpha(false);

	Label* label = new Label("");
	label->setID(1);
	root->add(label);
	label->setLabel(message);

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

ConfirmDialog::ConfirmDialog(const string& msg) {
	message = msg;
}

void ConfirmDialog::setMessage(const string& msg) {
	message = msg;
}
