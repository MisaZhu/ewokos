#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <Widget/Image.h>
#include <Widget/Label.h>
#include <Widget/LabelButton.h>
#include <Widget/List.h>
#include <Widget/EditLine.h>
#include <Widget/Grid.h>
#include <Widget/Scroller.h>
#include <Widget/Split.h>
#include <WidgetEx/LayoutWidget.h>
#include <WidgetEx/FileDialog.h>
#include <WidgetEx/ConfirmDialog.h>

#include <x++/X.h>
#include <unistd.h>
#include <font/font.h>

using namespace Ewok;

static void onMenuItemFunc(MenuItem* it, void* data) {
	klog("onMenuItemFunc: %d\n", it->id);
}

static void onEventFunc(Widget* wd, xevent_t* evt, void* arg) {
	if(evt->type == XEVT_MOUSE && evt->state == MOUSE_STATE_MOVE)
		return;
	klog("onEventFunc: id:%d: name: %s, type: %d, state: %d\n", 
			wd->getID(), wd->getName().c_str(), evt->type, evt->state);
}

int main(int argc, char** argv) {
	if(argc < 2) {
		klog("Usage: %s <xxxx.layout>\n", argv[0]);
		return 0;
	}
	
	X x;
	WidgetWin win;
	RootWidget* root = win.getRoot();

	LayoutWidget* layout = new LayoutWidget();
	layout->setMenuItemFunc(onMenuItemFunc);
	layout->setEventFunc(onEventFunc);
	layout->loadConfig(argv[1]); // 加载布局文件
	root->add(layout);

	win.open(&x, 0, -1, -1, 400, 300, argv[1], XWIN_STYLE_NORMAL);
	win.setTimer(16);

	widgetXRun(&x, &win);	
	return 0;
}