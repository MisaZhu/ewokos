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
		color = c;
		update();
	}

	uint32_t getColor() {
		return color;
	}
};

xwm_theme_t _xwm_theme;

static void loadTheme(LayoutWidget* layout) {
	int xserv_pid = dev_get_pid("/dev/x");
	if(xserv_pid < 0)
		return;

	proto_t out;
	PF->init(&out);
	if(dev_cntl_by_pid(xserv_pid, X_DCNTL_GET_XWM_THEME, NULL, &out) != 0) {
		return;
	}	
	proto_read_to(&out, &_xwm_theme, sizeof(xwm_theme_t));
	PF->clear(&out);

	Widget* wd = layout->get("desktop_bg_color");
	if(wd != NULL)  {
		ColorButton* btn = (ColorButton*)wd;
		btn->setColor(_xwm_theme.desktopBGColor);
	}

	wd = layout->get("frame_bg_top_color");
	if(wd != NULL)  {
		ColorButton* btn = (ColorButton*)wd;
		btn->setColor(_xwm_theme.frameBGTopColor);
	}

	wd = layout->get("frame_fg_top_color");
	if(wd != NULL)  {
		ColorButton* btn = (ColorButton*)wd;
		btn->setColor(_xwm_theme.frameFGTopColor);
	}
}

static void setTheme(LayoutWidget* layout) {
	int xserv_pid = dev_get_pid("/dev/x");
	if(xserv_pid < 0)
		return;

	Widget* wd = layout->get("desktop_bg_color");
	if(wd != NULL)  {
		ColorButton* btn = (ColorButton*)wd;
		_xwm_theme.desktopBGColor = btn->getColor();
	}

	wd = layout->get("frame_bg_top_color");
	if(wd != NULL)  {
		ColorButton* btn = (ColorButton*)wd;
		_xwm_theme.frameBGTopColor = btn->getColor();
	}

	wd = layout->get("frame_fg_top_color");
	if(wd != NULL)  {
		ColorButton* btn = (ColorButton*)wd;
		_xwm_theme.frameFGTopColor = btn->getColor();
	}

	proto_t in;
	PF->init(&in)->add(&in, &_xwm_theme, sizeof(xwm_theme_t));
	dev_cntl_by_pid(xserv_pid, X_DCNTL_SET_XWM_THEME, &in, NULL);
	PF->clear(&in);
}

static ColorDialog *_colorDialog = NULL;

static void onEventFunc(Widget* wd, xevent_t* evt, void* arg) {
	if(evt->type != XEVT_MOUSE || evt->state != MOUSE_STATE_CLICK)
		return;
	
	string name = wd->getName();
	if(name.find("_color") != string::npos) {
		ColorButton* btn = (ColorButton*)wd;
		_colorDialog->popup(wd->getWin(), 256, 160, "color", XWIN_STYLE_NO_RESIZE, btn);
		_colorDialog->setColor(btn->getColor());
	}
	else if(name == "okButton") {
		setTheme((LayoutWidget*)arg);
		wd->getWin()->close();
	}
	else if(name == "cancelButton") {
		wd->getWin()->close();
	}
}

static Widget* createByTypeFunc(const string& type) {
	if(type == "ColorButton") {
		ColorButton* btn = new ColorButton();
		return btn;
	}
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

	win.open(&x, 0, -1, -1, 300, 300, "xwm_theme", XWIN_STYLE_NO_RESIZE);
	win.setTimer(16);

	_colorDialog = new ColorDialog();

	loadTheme(layout);
	widgetXRun(&x, &win);	

	delete _colorDialog;
	return 0;
}