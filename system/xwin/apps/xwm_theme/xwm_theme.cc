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

	Widget* wd = layout->getChild("desktop_bg_color");
	if(wd != NULL)  {
		ColorButton* btn = (ColorButton*)wd;
		btn->setColor(_xwm_theme.desktopBGColor);
	}

	wd = layout->getChild("frame_bg_color");
	if(wd != NULL)  {
		ColorButton* btn = (ColorButton*)wd;
		btn->setColor(_xwm_theme.frameBGColor);
	}

	wd = layout->getChild("frame_fg_color");
	if(wd != NULL)  {
		ColorButton* btn = (ColorButton*)wd;
		btn->setColor(_xwm_theme.frameFGColor);
	}

	wd = layout->getChild("font");
	if(wd != NULL)  {
		LabelButton* btn = (LabelButton*)wd;
		btn->setLabel(_xwm_theme.fontName);
	}

	wd = layout->getChild("desktop_image");
	if(wd != NULL)  {
		LabelButton* btn = (LabelButton*)wd;
		btn->setLabel(_xwm_theme.patternName);
	}

	wd = layout->getChild("image");
	if(wd != NULL)  {
		Image* img = (Image*)wd;
		img->dupTheme(layout->getWin()->getTheme());
		img->getTheme()->basic.bgColor = _xwm_theme.desktopBGColor;
		img->loadImage(_xwm_theme.patternName);
	}
}

static void setTheme(LayoutWidget* layout) {
	int xserv_pid = dev_get_pid("/dev/x");
	if(xserv_pid < 0)
		return;

	Widget* wd = layout->getChild("desktop_bg_color");
	if(wd != NULL)  {
		ColorButton* btn = (ColorButton*)wd;
		_xwm_theme.desktopBGColor = btn->getColor();
	}

	wd = layout->getChild("frame_bg_color");
	if(wd != NULL)  {
		ColorButton* btn = (ColorButton*)wd;
		_xwm_theme.frameBGColor = btn->getColor();
	}

	wd = layout->getChild("frame_fg_color");
	if(wd != NULL)  {
		ColorButton* btn = (ColorButton*)wd;
		_xwm_theme.frameFGColor = btn->getColor();
	}

	wd = layout->getChild("font");
	if(wd != NULL)  {
		LabelButton* btn = (LabelButton*)wd;
		memset(_xwm_theme.fontName, 0, FONT_NAME_MAX);
		strncpy(_xwm_theme.fontName, btn->getLabel().c_str(), FONT_NAME_MAX-1);
	}

	wd = layout->getChild("desktop_image");
	if(wd != NULL)  {
		LabelButton* btn = (LabelButton*)wd;
		memset(_xwm_theme.patternName, 0, THEME_NAME_MAX);
		strncpy(_xwm_theme.patternName, btn->getLabel().c_str(), THEME_NAME_MAX-1);
	}

	proto_t in;
	PF->init(&in)->add(&in, &_xwm_theme, sizeof(xwm_theme_t));
	dev_cntl_by_pid(xserv_pid, X_DCNTL_SET_XWM_THEME, &in, NULL);
	PF->clear(&in);
}

static ColorDialog *_colorDialog = NULL;
static FontDialog *_fontDialog = NULL;
static FileDialog *_fileDialog = NULL;

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
	else if(name == "desktop_image") {
		LabelButton* btn = (LabelButton*)wd;
		string fname = btn->getLabel();
		char dir[FS_FULL_NAME_MAX];
		vfs_dir_name(fname.c_str(), dir, FS_FULL_NAME_MAX);
		_fileDialog->setInitPath(dir);
		_fileDialog->popup(wd->getWin(), 320, 320, "image", XWIN_STYLE_NO_RESIZE, wd);
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
		btn->setColor((color & 0x00ffffff) | (alpha << 24));
		if(btn->getName() == "desktop_bg_color") {
			LayoutWidget* layout = ((LayoutWin*)xwin)->getLayoutWidget();
			if(layout == NULL)
				return;
			Image* img = (Image*)layout->getChild("image");
			if(img!= NULL) {
				img->getTheme()->basic.bgColor = color;
				img->update();
			}
		}
	}
	else if(from == _fontDialog) {
		LabelButton* btn = (LabelButton*)arg;
		btn->setLabel(_fontDialog->getResult());
	}
	else if(from == _fileDialog) {
		LabelButton* btn = (LabelButton*)arg;
		string res = _fileDialog->getResult();
		btn->setLabel(_fileDialog->getResult());
		LayoutWidget* layout = ((LayoutWin*)xwin)->getLayoutWidget();
		if(layout == NULL)
			return;

		Image* img = (Image*)layout->getChild("image");
		if(img!= NULL) {
			img->loadImage(res.c_str());
		}
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

	win.open(&x, -1, -1, -1, 280, 300, "xwm_theme", XWIN_STYLE_NORMAL);
	win.setTimer(16);

	_colorDialog = new ColorDialog();
	_fontDialog = new FontDialog();
	_fileDialog = new FileDialog();

	loadTheme(layout);
	widgetXRun(&x, &win);	

	delete _colorDialog;
	delete _fontDialog;
	delete _fileDialog;
	return 0;
}