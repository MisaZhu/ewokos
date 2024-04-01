#include <Widget/WidgetWin.h>
#include <Widget/Image.h>
#include <Widget/Label.h>
#include <Widget/LabelButton.h>
#include <Widget/List.h>
#include <x++/X.h>
#include <unistd.h>
#include <font/font.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/kernel_tic.h>
#include <upng/upng.h>

#include <string>
using namespace EwokSTL;

using namespace Ewok;

class FontDemo: public Widget {
protected:
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.bgColor);

		font_t* f = font;
		int y = 10;
		if(f != NULL) {
			graph_draw_text_font(g, r.x+10, y, "abcdefghijklmn", f, theme->basic.fgColor);
			y += 20;
			graph_draw_text_font(g, r.x+10, y, "opqrstuvwxyz", f, theme->basic.fgColor);
			y += 20;
			graph_draw_text_font(g, r.x+10, y, "0123456789.+-", f, theme->basic.fgColor);
			y += 20;
			graph_draw_text_font(g, r.x+10, y, "~!@#$%%^&*()", f, theme->basic.fgColor);
			y += 20;
			graph_draw_text_font(g, r.x+10, y, "中文字体演示", f, theme->basic.fgColor);
			y += 20;
		}
		y += 20;

		f = fontBig;
		if(f != NULL) {
			graph_draw_text_font(g, r.x+10, y, "abcdefghijklmn", f, theme->basic.fgColor);
			y += 40;
			graph_draw_text_font(g, r.x+10, y, "opqrstuvwxyz", f, theme->basic.fgColor);
			y += 40;
			graph_draw_text_font(g, r.x+10, y, "0123456789.+-", f, theme->basic.fgColor);
			y += 40;
			graph_draw_text_font(g, r.x+10, y, "~!@#$%%^&*()", f, theme->basic.fgColor);
			y += 40;
			graph_draw_text_font(g, r.x+10, y, "中文字体演示", f, theme->basic.fgColor);
			y += 40;
		}
	}

	font_t* font;
	font_t* fontBig;
public: 
	FontDemo() {
		font = NULL;
		fontBig = NULL;
	}
	~FontDemo() {
		if(font != NULL)
			font_free(font);
		if(fontBig != NULL)
			font_free(fontBig);
	}

	void setFont(const string& fontName) {
		if(font != NULL)
			font_free(font);
		if(fontBig != NULL)
			font_free(fontBig);

		font = font_new(fontName.c_str(), 14, false);
		fontBig = font_new(fontName.c_str(), 32, false);
		update();
	}
};


class FontList: public List {
	static const int32_t MAX_FONTS = 32;
	string fonts[MAX_FONTS];
	FontDemo* demo;
protected:
	void drawBG(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.titleBGColor, false);
	}

	void drawItem(graph_t* g, XTheme* theme, int32_t index, const grect_t& r) {
		if(index >= MAX_FONTS)
			return;

		uint32_t color = theme->basic.fgColor;
		if(index == itemSelected) {
			graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.selectBGColor);
			color = theme->basic.selectColor;
		}
		graph_draw_text_font(g, r.x+2, r.y+2, fonts[index].c_str(), theme->getFont(), color);
	}

	void onSelect(int index) {
		if(demo == NULL)
			return;
		demo->setFont(fonts[index].c_str());
	}
public:
	FontList() {
		demo = NULL;
	}

	void loadFont() {
		proto_t ret;
		PF->init(&ret);
		if(dev_cntl("/dev/font", FONT_DEV_LIST, NULL, &ret) != 0)
			return;
		
		uint32_t i;
		for(i=0; i<MAX_FONTS; i++) {
			const char* s = proto_read_str(&ret);
			if(s == NULL || s[0] == 0)
				break;
			fonts[i] = s;
		}
		PF->clear(&ret);
		setItemNum(i);
	}

	void setDemo(FontDemo* demo) {
		this->demo = demo;
	}
};

int main(int argc, char** argv) {
	X x;
	WidgetWin win;
	RootWidget* root = new RootWidget();
	win.setRoot(root);
	root->setType(Container::HORIZONTAL);
	root->setAlpha(false);

	FontList* list = new FontList();
	list->loadFont();
	list->setItemSize(20);
	list->fix(160, 0);
	root->add(list);

	Scroller* scrollerV = new Scroller();
	scrollerV->fix(8, 0);
	root->add(scrollerV);
	list->setScrollerV(scrollerV);

	FontDemo* demo = new FontDemo();
	root->add(demo);
	list->setDemo(demo);

	x.open(0, &win, -1, -1, 460, 460, "xfont", XWIN_STYLE_NORMAL);
	win.setVisible(true);
	x.run(NULL, &win);
	return 0;
}