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
#include <WidgetEx/ColorDialog.h>
#include <WidgetEx/ConfirmDialog.h>

#include <WidgetEx/LayoutWin.h>

#include <x++/X.h>
#include <unistd.h>
#include <font/font.h>

using namespace Ewok;

class ColorButton : public Button {
	uint32_t color;
protected:
	virtual void paintPanel(graph_t* g, XTheme* theme, const grect_t& rect) {
		graph_fill(g, rect.x, rect.y, rect.w, rect.h, color);
	}
public:
	ColorButton() {
		color = 0xff000000;
	}

	void setColor(uint32_t c) {
		klog("color: %x\n", c);
		color = c;
		update();
	}
};

static ColorDialog *_colorDialog = NULL;

static void onEventFunc(Widget* wd, xevent_t* evt, void* arg) {
	if(evt->type != XEVT_MOUSE || evt->state != MOUSE_STATE_CLICK)
		return;
	
	uint32_t id = wd->getID();
	if(id > 0) {
		_colorDialog->popup(wd->getWin(), 256, 160, "color", XWIN_STYLE_NO_RESIZE, wd);
	}
}

static Widget* createByTypeFunc(const string& type) {
	if(type == "ColorButton")
		return new ColorButton();
	return NULL;
}

static void _dialogedFunc(XWin* xwin, XWin* from, int res, void* arg) {
	if(res != Dialog::RES_OK)
		return;

	if(from == _colorDialog) {
		uint32_t color = _colorDialog->getColor();
		uint8_t alpha = _colorDialog->getTransparent();
		ColorButton* btn = (ColorButton*)arg;
		btn->setColor((color & 0x00ffffff) | (alpha << 24));
	}
}

int main(int argc, char** argv) {
	X x;
	LayoutWin win;
	LayoutWidget* layout = win.getLayoutWidget();
	layout->setEventFunc(onEventFunc);
	layout->setCreateByTypeFunc(createByTypeFunc);

	win.loadConfig(X::getResName("layout.xwl")); // 加载布局文件
	win.setOnDialogedFunc(_dialogedFunc);

	win.open(&x, 0, -1, -1, 300, 300, "xtheme", XWIN_STYLE_NORMAL);
	win.setTimer(16);

	_colorDialog = new ColorDialog();

	widgetXRun(&x, &win);	

	delete _colorDialog;
	return 0;
}