#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <Widget/Text.h>
#include <Widget/Label.h>
#include <WidgetEx/Menubar.h>
#include <WidgetEx/Menu.h>
#include <Widget/LabelButton.h>
#include <WidgetEx/FileDialog.h>
#include <WidgetEx/FontDialog.h>
#include <x++/X.h>
#include <unistd.h>
#include <ewoksys/keydef.h>

using namespace Ewok;

class StatusLabel: public Label {
protected:
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.titleBGColor, true);
		font_t* font = theme->getFont();
		int y = r.y + (r.h- font_get_height(font, theme->basic.fontSize))/2;
		graph_draw_text_font(g, r.x+4, y, label.c_str(), font, theme->basic.fontSize, theme->basic.titleColor);
	}
public:
	StatusLabel(const char* label) : Label(label) {
		alpha = false;
	}
};

class MyText: public Text {
protected:
	void updateScroller() {
		Text::updateScroller();

		char s[128] = { 0 };
		if(contentSize > 0) {
			int curr = offset+pageSize;
			if(curr > contentSize)
				curr = contentSize;
			snprintf(s, 127, "%.2f%% font_size:%d", 100.0 * (float)(curr)/(float)contentSize, getFontSize());
		}
		else
			snprintf(s, 127, "--%%");
		statusLabel->setLabel(s);
	}
	
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		Text::onRepaint(g, theme, r);
	}


	bool onIM(xevent_t* ev) {
		if(Text::onIM(ev))
			return true;

		if(ev->state != XIM_STATE_PRESS)
			return false;

		if(ev->value.im.value == KEY_RIGHT ||
				ev->value.im.value == JOYSTICK_RIGHT) {
			setFontSize(getFontSize()+4);
			return true;
		}
		if(ev->value.im.value == KEY_LEFT ||
				ev->value.im.value == JOYSTICK_LEFT) {
			setFontSize(getFontSize()-4);
			return true;
		}
		return false;
}
public:
	StatusLabel* statusLabel;
	MyText() {
		statusLabel = NULL;
	}
};


class TextWin: public WidgetWin{
	FileDialog fdialog;
	FontDialog fontdialog;

	void loadFile(const string& fname) {
		if(text == NULL)
			return;
		
		busy(true);

		int sz;
		char* content = (char*)vfs_readfile(fname.c_str(), &sz);
		if(content != NULL) {
			text->setContent(content, sz);
			free(content);
		}
		busy(false);
	}
protected:
	void onDialoged(XWin* from, int res) {
		if(res == Dialog::RES_OK) {
			if(from == &fdialog) {
				string fname = fdialog.getResult();
				loadFile(fname.c_str());
			}
			else if(from == &fontdialog) {
				string fontName = fontdialog.getResult();
				text->setFont(fontName);
			}
			repaint();
		}
	}
public:
	MyText* text;

	void loadConfig(void) {
		const char* fname = X::getResName("config.json");
		json_var_t *conf_var = json_parse_file(fname);
		text->setFont(json_get_str_def(conf_var, "font", DEFAULT_SYSTEM_FONT));
		json_var_unref(conf_var);
	}

	void load(const string& fname) {
		if(fname.length() == 0)
			fdialog.popup(this, 400, 300, "files", XWIN_STYLE_NORMAL);
		else 
			loadFile(fname);
	}

	void font(const string& fontName) {
		if(fontName.length() == 0)
			fontdialog.popup(this, 400, 300, "fonts", XWIN_STYLE_NORMAL);
	}
};

static void onLoadFunc(MenuItem* it, void* p) {
	TextWin* win = (TextWin*)p;
	win->load("");
}

static void onFontFunc(MenuItem* it, void* p) {
	TextWin* win = (TextWin*)p;
	win->font("");
}

static void onQuitFunc(MenuItem* it, void* p) {
	TextWin* win = (TextWin*)p;
	win->close();
}

static void onZoomInClickFunc(Widget* wd) {
	TextWin* win = (TextWin*)wd->getWin();
	uint32_t size = win->text->getFontSize();
	win->text->setFontSize(size+4);
}

static void onZoomOutClickFunc(Widget* wd) {
	TextWin* win = (TextWin*)wd->getWin();
	uint32_t size = win->text->getFontSize();
	win->text->setFontSize(size-4);
}

int main(int argc, char** argv) {
	X x;
	TextWin win;
	RootWidget* root = new RootWidget();
	win.setRoot(root);
	root->setType(Container::VERTICLE);
	

	Container* c = new Container();
	c->setType(Container::HORIZONTAL);
	c->fix(0, 20);
	root->add(c);

	Menu* menu = new Menu();
	menu->add("open", NULL, NULL, onLoadFunc, &win);
	menu->add("font", NULL, NULL, onFontFunc, &win);
	menu->add("quit", NULL, NULL, onQuitFunc, &win);
	Menubar* menubar = new Menubar();
	menubar->add("file", NULL, menu, NULL, NULL);
	c->add(menubar);

	LabelButton* zoomInButton = new LabelButton("+");
	zoomInButton->onClickFunc = onZoomInClickFunc;
	zoomInButton->fix(48, 0);
	c->add(zoomInButton);

	LabelButton* zoomOutButton = new LabelButton("-");
	zoomOutButton->onClickFunc = onZoomOutClickFunc;
	zoomOutButton->fix(48, 0);
	c->add(zoomOutButton);

	c = new Container();
	c->setType(Container::HORIZONTAL);
	root->add(c);

	MyText* text = new MyText();
	c->add(text);
	root->focus(text);
	win.text = text;

	Scroller* scrollerV = new Scroller();
	scrollerV->fix(8, 0);
	c->add(scrollerV);
	text->setScrollerV(scrollerV);

	StatusLabel* statusLabel = new StatusLabel("");
	statusLabel->fix(0, 20);
	root->add(statusLabel);
	text->statusLabel = statusLabel;

	win.loadConfig();
	win.open(&x, 0, -1, -1, 460, 460, "xread", XWIN_STYLE_NORMAL);

	if(argc >= 2) {
		win.load(argv[1]);
		win.repaint();
	}

	widgetXRun(&x, &win);
	return 0;
}