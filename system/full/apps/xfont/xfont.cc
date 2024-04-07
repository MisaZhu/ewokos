#include <Widget/WidgetWin.h>
#include <Widget/Image.h>
#include <Widget/Label.h>
#include <Widget/LabelButton.h>
#include <Widget/List.h>
#include <Widget/Split.h>
#include <x++/X.h>
#include <unistd.h>
#include <font/font.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/kernel_tic.h>
#include <upng/upng.h>

#include <string>
using namespace EwokSTL;

using namespace Ewok;

class FontDemo: public Scrollable {
	int off_y;
	uint32_t demoH;
	int last_mouse_down;
protected:
	font_t* font;
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.bgColor);

		if(font == NULL)
			return;

		uint32_t y = 0;
		uint16_t margin = 4;
		for(int i=0; i<4; i++) {
			uint16_t size = (i+1) * 12;
			graph_draw_text_font(g, r.x+10, y - off_y, "abcdefghijklmn", font, size, theme->basic.fgColor);
			y += size + margin;
			graph_draw_text_font(g, r.x+10, y - off_y, "opqrstuvwxyz", font, size, theme->basic.fgColor);
			y += size + margin;
			graph_draw_text_font(g, r.x+10, y - off_y, "0123456789.+-", font, size, theme->basic.fgColor);
			y += size + margin;
			graph_draw_text_font(g, r.x+10, y - off_y, "~!@#$%%^&*()", font, size, theme->basic.fgColor);
			y += size + margin;
			graph_draw_text_font(g, r.x+10, y - off_y, "中文字体演示", font, size, theme->basic.fgColor);
			y += (size + margin)*2;
		}

		demoH = y;
		updateScroller();
	}

	bool onScroll(int step, bool horizontal) {
		int old_off = off_y;
		off_y -= step;

		if(step < 0) {
			if((off_y + area.h) >= demoH)
				off_y = demoH - area.h;
		}

		if(off_y < 0)
			off_y = 0;
		
		if(off_y == old_off)
			return false;

		return true;
	}

	void updateScroller() {
		if(demoH <= area.h && off_y > 0)
			setScrollerInfo(area.h + off_y, off_y, area.h, false);
		else
			setScrollerInfo(demoH, off_y, area.h, false);
	}

	bool onMouse(xevent_t* ev) {
		gpos_t ipos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
		if(ev->state == XEVT_MOUSE_DOWN) {
			last_mouse_down = ipos.y;
			update();
		}
		else if(ev->state == XEVT_MOUSE_DRAG) {
			int pos = ipos.y;
			int mv = (pos - last_mouse_down) * 2;
			if(abs_32(mv) > 0) {
				last_mouse_down = pos;
				scroll(mv, false);
			}
		}
		else if(ev->state == XEVT_MOUSE_MOVE) {
			if(ev->value.mouse.button == MOUSE_BUTTON_SCROLL_UP) {
				scroll(-16, false);
			}
			else if(ev->value.mouse.button == MOUSE_BUTTON_SCROLL_DOWN) {
				scroll(16, false);
			}
		}
		return true;
	}

public: 
	FontDemo() {
		font = NULL;
		off_y = 0;
		demoH = 0;
	}

	~FontDemo() {
		if(font != NULL)
			font_free(font);
	}

	void setFont(const string& fontName) {
		if(font != NULL)
			font_free(font);

		font = font_new(fontName.c_str(), false);
		off_y = 0;
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
		graph_draw_text_font(g, r.x+2, r.y+2, fonts[index].c_str(), theme->getFont(), theme->basic.fontSize, color);
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

	Split* split = new Split();
	split->attach(list);
	root->add(split);

	FontDemo* demo = new FontDemo();
	root->add(demo);
	list->setDemo(demo);

	scrollerV = new Scroller();
	scrollerV->fix(8, 0);
	root->add(scrollerV);
	demo->setScrollerV(scrollerV);

	win.open(&x, 0, -1, -1, 460, 460, "xfont", XWIN_STYLE_NORMAL);
	x.run(NULL, &win);
	return 0;
}