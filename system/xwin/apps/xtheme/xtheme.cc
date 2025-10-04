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
#include <WidgetEx/FontDialog.h>
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

x_theme_t _xtheme;

static void loadTheme(LayoutWidget* layout) {
	int xserv_pid = dev_get_pid("/dev/x");
	if(xserv_pid < 0)
		return;

	proto_t out;
	PF->init(&out);
	if(dev_cntl_by_pid(xserv_pid, X_DCNTL_GET_THEME, NULL, &out) != 0) {
		return;
	}	
	proto_read_to(&out, &_xtheme, sizeof(x_theme_t));
	PF->clear(&out);

	Widget* wd = layout->getChild("bg_color");
	if(wd != NULL)  {
		ColorButton* btn = (ColorButton*)wd;
		btn->setColor(_xtheme.bgColor);
	}

	wd = layout->getChild("fg_color");
	if(wd != NULL)  {
		ColorButton* btn = (ColorButton*)wd;
		btn->setColor(_xtheme.fgColor);
	}

	wd = layout->getChild("doc_bg_color");
	if(wd != NULL)  {
		ColorButton* btn = (ColorButton*)wd;
		btn->setColor(_xtheme.docBGColor);
	}

	wd = layout->getChild("doc_fg_color");
	if(wd != NULL)  {
		ColorButton* btn = (ColorButton*)wd;
		btn->setColor(_xtheme.docFGColor);
	}

	wd = layout->getChild("font");
	if(wd != NULL)  {
		LabelButton* btn = (LabelButton*)wd;
		btn->setLabel(_xtheme.fontName);
	}
}

static void setTheme(LayoutWidget* layout) {
	int xserv_pid = dev_get_pid("/dev/x");
	if(xserv_pid < 0)
		return;

	Widget* wd = layout->getChild("bg_color");
	if(wd != NULL)  {
		ColorButton* btn = (ColorButton*)wd;
		_xtheme.bgColor = btn->getColor();
	}

	wd = layout->getChild("fg_color");
	if(wd != NULL)  {
		ColorButton* btn = (ColorButton*)wd;
		_xtheme.fgColor = btn->getColor();
	}

	wd = layout->getChild("doc_bg_color");
	if(wd != NULL)  {
		ColorButton* btn = (ColorButton*)wd;
		_xtheme.docBGColor = btn->getColor();
	}

	wd = layout->getChild("doc_fg_color");
	if(wd != NULL)  {
		ColorButton* btn = (ColorButton*)wd;
		_xtheme.docFGColor = btn->getColor();
	}

	wd = layout->getChild("font");
	if(wd != NULL)  {
		LabelButton* btn = (LabelButton*)wd;
		memset(_xtheme.fontName, 0, FONT_NAME_MAX);
		strncpy(_xtheme.fontName, btn->getLabel().c_str(), FONT_NAME_MAX-1);
	}

	_xtheme.uuid++;
	proto_t in;
	PF->init(&in)->add(&in, &_xtheme, sizeof(x_theme_t));
	dev_cntl_by_pid(xserv_pid, X_DCNTL_SET_THEME, &in, NULL);
	PF->clear(&in);
}

static ColorDialog *_colorDialog = NULL;
static FontDialog *_fontDialog = NULL;

static void onEventFunc(Widget* wd, xevent_t* evt, void* arg) {
	if(evt->type != XEVT_MOUSE || evt->state != MOUSE_STATE_CLICK)
		return;
	
	string name = wd->getName();
	if(name.find("_color") != string::npos) {
		ColorButton* btn = (ColorButton*)wd;
		_colorDialog->popup(wd->getWin(), 256, 160, "color", XWIN_STYLE_NO_RESIZE, btn);
		_colorDialog->setColor(btn->getColor());

	}
	else if(name == "font") {
		_fontDialog->popup(wd->getWin(), 320, 320, "font", XWIN_STYLE_NO_RESIZE, wd);
	}
	else if(name == "okButton") {
		setTheme((LayoutWidget*)arg);
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

		color = (color & 0x00ffffff) | (alpha << 24);
		btn->setColor(color);

		string name = btn->getName();
		WidgetWin* win = btn->getWin();
		XTheme* theme = win->getTheme();
		if(name == "bg_color")
			theme->basic.bgColor = color;
		else if(name == "fg_color")
			theme->basic.fgColor = color;
		else if(name == "doc_bg_color")
			theme->basic.docBGColor = color;
		else if(name == "doc_fg_color")
			theme->basic.docFGColor = color;
		
		win->getRoot()->update();
		win->repaint();
	}
	else if(from == _fontDialog) {
		LabelButton* btn = (LabelButton*)arg;
		btn->setLabel(_fontDialog->getResult());
		WidgetWin* win = btn->getWin();
		XTheme* theme = win->getTheme();
		memset(theme->basic.fontName, 0, FONT_NAME_MAX);
		strncpy(theme->basic.fontName, _fontDialog->getResult().c_str(), FONT_NAME_MAX-1);
		theme->setFont(theme->basic.fontName, theme->basic.fontSize);
		win->getRoot()->update();
		win->repaint();
	}
}

int main(int argc, char** argv) {

	X x;
	LayoutWin win;
	LayoutWidget* layout = win.getLayoutWidget();
	layout->setEventFunc(onEventFunc);
	layout->setCreateByTypeFunc(createByTypeFunc);

	win.loadConfig(X::getResName("layout.xwl").c_str()); // 加载布局文件
	win.setOnDialogedFunc(_dialogedFunc);

	win.open(&x, -1, -1, -1, 280, 300, "xtheme", XWIN_STYLE_NORMAL);
	win.setTimer(16);

	_colorDialog = new ColorDialog();
	_fontDialog = new FontDialog();

	loadTheme(layout);
	widgetXRun(&x, &win);	

	delete _colorDialog;
	delete _fontDialog;
	return 0;
}