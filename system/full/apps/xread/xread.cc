#include <Widget/WidgetWin.h>
#include <Widget/Text.h>
#include <Widget/Label.h>
#include <WidgetEx/Menubar.h>
#include <WidgetEx/Menu.h>
#include <Widget/LabelButton.h>
#include <WidgetEx/FileDialog.h>
#include <x++/X.h>
#include <unistd.h>

using namespace Ewok;

class StatusLabel: public Label {
protected:
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.titleBGColor, true);
		font_t* font = theme->getFont();
		int y = r.y + (r.h-font_get_inst_h(font, theme->basic.fontSize))/2;
		graph_draw_text_font(g, r.x+4, y, label.c_str(), font, theme->basic.fontSize, theme->basic.titleColor);
	}
public:
	StatusLabel(const char* label) : Label(label) {}
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
public:
	StatusLabel* statusLabel;
	MyText() {
		statusLabel = NULL;
	}
};


class TextWin: public WidgetWin{
	FileDialog fdialog;

	void loadFile(const string& fname) {
		if(text == NULL)
			return;

		int sz;
		char* content = (char*)vfs_readfile(fname.c_str(), &sz);
		if(content != NULL) {
			text->setContent(content, sz);
			free(content);
		}
	}
protected:
	void onDialoged(XWin* from, int res) {
		if(res == Dialog::RES_OK) {
			string fname = fdialog.getResult();
			loadFile(fname.c_str());
			repaint();
		}
	}
public:
	MyText* text;

	void load(const string& fname) {
		if(fname.length() == 0)
			fdialog.popup(this, 400, 300, "files", XWIN_STYLE_NORMAL);
		else 
			loadFile(fname);
	}
};

static void onLoadFunc(MenuItem* it, void* p) {
	TextWin* win = (TextWin*)p;
	win->load("");
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
	root->setAlpha(false);

	Menu* menu = new Menu();
	menu->add("open", NULL, NULL, onLoadFunc, &win);
	menu->add("quit", NULL, NULL, onQuitFunc, &win);

	Menubar* menubar = new Menubar();
	menubar->add("file", NULL, menu, NULL, NULL);
	menubar->fix(0, 20);
	root->add(menubar);

	Container* c = new Container();
	c->setType(Container::HORIZONTAL);
	root->add(c);

	MyText* text = new MyText();
	c->add(text);
	win.text = text;

	Scroller* scrollerV = new Scroller();
	scrollerV->fix(8, 0);
	c->add(scrollerV);
	text->setScrollerV(scrollerV);

	StatusLabel* statusLabel = new StatusLabel("");
	statusLabel->fix(0, 20);
	root->add(statusLabel);
	text->statusLabel = statusLabel;

	c = new Container();
	c->setType(Container::HORIZONTAL);
	c->fix(0, 22);
	root->add(c);

	LabelButton* zoomInButton = new LabelButton("+");
	zoomInButton->onClickFunc = onZoomInClickFunc;
	c->add(zoomInButton);

	LabelButton* zoomOutButton = new LabelButton("-");
	zoomOutButton->onClickFunc = onZoomOutClickFunc;
	c->add(zoomOutButton);

	win.getTheme()->setFont("system.cn", 14);
	if(argc >= 2)
		win.load(argv[1]);

	win.open(&x, 0, -1, -1, 460, 460, "xtext", XWIN_STYLE_NORMAL);
	x.run(NULL, &win);
	return 0;
}