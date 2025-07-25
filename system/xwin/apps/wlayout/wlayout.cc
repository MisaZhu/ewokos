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

int main(int argc, char** argv) {
	if(argc < 2) {
		klog("Usage: %s <xxxx.layout>\n", argv[0]);
		return 0;
	}
	
	X x;
	WidgetWin win;
	RootWidget* root = new RootWidget();
	win.setRoot(root);
	root->setType(Container::VERTICLE);

	LayoutWidget* layout = new LayoutWidget();
	layout->loadConfig(argv[1]); // 加载布局文件
	root->add(layout);

	win.open(&x, 0, -1, -1, 400, 300, argv[1], XWIN_STYLE_NORMAL);
	win.setTimer(16);

	widgetXRun(&x, &win);	
	return 0;
}