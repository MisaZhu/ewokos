#include <Widget/WidgetX.h>
#include <Widget/Image.h>
#include <Widget/Label.h>
#include <Widget/LabelButton.h>
#include <Widget/List.h>
#include <Widget/EditLine.h>
#include <Widget/Grid.h>
#include <Widget/Scroller.h>
#include <Widget/Splitter.h>
#include <WidgetEx/LayoutWidget.h>
#include <WidgetEx/FileDialog.h>
#include <WidgetEx/ConfirmDialog.h>

#include <WidgetEx/LayoutWin.h>

#include <x++/X.h>
#include <unistd.h>
#include <font/font.h>

using namespace Ewok;

static void onMenuItemFunc(MenuItem* it, void* data) {
	slog("onMenuItemFunc: %d\n", it->id);
}

static void onEventFunc(Widget* wd, xevent_t* evt, void* arg) {
	if(evt->type == XEVT_MOUSE && evt->state == MOUSE_STATE_MOVE)
		return;
	slog("onEventFunc: id:%d: name: %s, type: %d, state: %d\n", 
			wd->getID(), wd->getName().c_str(), evt->type, evt->state);
}

int main(int argc, char** argv) {
	if(argc < 2) {
		slog("Usage: %s <xxxx.layout>\n", argv[0]);
		return 0;
	}
	
	X x;
	LayoutWin win;
	LayoutWidget* layout = win.getLayoutWidget();
	layout->setMenuItemFunc(onMenuItemFunc);
	layout->setEventFunc(onEventFunc);
	win.loadConfig(argv[1]); // 加载布局文件

	win.open(&x, -1, -1, -1, 0, 0, argv[1], XWIN_STYLE_NORMAL);
	win.setTimer(16);

	widgetXRun(&x, &win);	
	return 0;
}