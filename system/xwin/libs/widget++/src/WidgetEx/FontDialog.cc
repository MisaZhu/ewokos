#include <WidgetEx/FontDialog.h>
#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
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
#include <graph/graph_png.h>

#include <string>
using namespace std;

using namespace Ewok;

class FontDemo: public Scrollable {
	int off_y;
	uint32_t demoH;
	int last_mouse_down;
protected:
	font_t* font;
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.docBGColor);

		if(font == NULL)
			return;
		getWin()->busy(true);

		uint32_t y = 0;
		uint16_t margin = 4;
		/*
		for(int i=0; i<4; i++) {
			uint16_t size = (i+1) * 8;
			graph_draw_text_font(g, r.x+10, y - off_y, "abcdefghijklmn", font, size, theme->basic.docFGColor);
			y += size + margin;
			graph_draw_text_font(g, r.x+10, y - off_y, "opqrstuvwxyz", font, size, theme->basic.docFGColor);
			y += size + margin;
			graph_draw_text_font(g, r.x+10, y - off_y, "0123456789.+-", font, size, theme->basic.docFGColor);
			y += size + margin;
			graph_draw_text_font(g, r.x+10, y - off_y, "~!@#$%%^&*()", font, size, theme->basic.docFGColor);
			y += size + margin;
			graph_draw_text_font(g, r.x+10, y - off_y, "中文字体演示", font, size, theme->basic.docFGColor);
			y += (size + margin)*2;
		}
		*/

		uint16_t size = 16;
		graph_draw_text_font(g, r.x+10, y - off_y, "abcdefghijklmn", font, size, theme->basic.docFGColor);
		y += size + margin;
		graph_draw_text_font(g, r.x+10, y - off_y, "opqrstuvwxyz", font, size, theme->basic.docFGColor);
		y += size + margin;
		graph_draw_text_font(g, r.x+10, y - off_y, "0123456789.+-", font, size, theme->basic.docFGColor);
		y += size + margin;
		graph_draw_text_font(g, r.x+10, y - off_y, "~!@#$%%^&*()", font, size, theme->basic.docFGColor);
		y += size + margin;
		graph_draw_text_font(g, r.x+10, y - off_y, "中文字体演示", font, size, theme->basic.docFGColor);
		y += (size + margin)*2;

		demoH = y;
		updateScroller();

		getWin()->busy(false);
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
		if(ev->state == MOUSE_STATE_DOWN) {
			last_mouse_down = ipos.y;
			update();
		}
		else if(ev->state == MOUSE_STATE_DRAG) {
			int pos = ipos.y;
			int mv = (pos - last_mouse_down) * 2;
			if(abs_32(mv) > 0) {
				last_mouse_down = pos;
				scroll(mv, false);
			}
		}
		else if(ev->state == MOUSE_STATE_MOVE) {
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
	}

	void setFont(font_t* fnt) {
		font = fnt;
		off_y = 0;
		update();
	}
};

typedef struct {
	string name;
	font_t* font;
} FontItem_t;

class FontList: public List {
	FontDialog* dialog;
	static const int32_t MAX_FONTS = 32;
	FontItem_t fonts[MAX_FONTS];
	FontDemo* demo;
protected:
	void drawBG(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.bgColor, false);
	}

	void drawItem(graph_t* g, XTheme* theme, int32_t index, const grect_t& r) {
		if(index >= MAX_FONTS)
			return;

		uint32_t color = theme->basic.fgColor;
		if(index == itemSelected) {
			graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.selectBGColor);
			color = theme->basic.selectColor;
		}
		graph_draw_text_font(g, r.x+2, r.y+2, fonts[index].name.c_str(), theme->getFont(), theme->basic.fontSize, color);
	}

	void onSelect(int index) {
		if(demo == NULL)
			return;
		dialog->setFontName(fonts[index].name);
		demo->setFont(fonts[index].font);
	}
public:
	FontList(FontDialog* dialog) {
		this->dialog = dialog;
		demo = NULL;
		for(int i=0; i<MAX_FONTS; i++) {
			fonts[i].name = "";
			fonts[i].font = NULL;
		}
	}

	~FontList() {
		demo = NULL;
		for(int i=0; i<MAX_FONTS; i++) {
			if(fonts[i].font != NULL)
				font_free(fonts[i].font);
		}
	}

	void loadFont() {
		setItemNum(0);
		proto_t ret;
		PF->init(&ret);
		if(dev_cntl("/dev/font", FONT_DEV_LIST, NULL, &ret) != 0)
			return;
		
		uint32_t i;
		for(i=0; i<MAX_FONTS; i++) {
			const char* s = proto_read_str(&ret);
			if(s == NULL || s[0] == 0)
				break;
			fonts[i].name = s;
			fonts[i].font = font_new(s, false);
		}
		PF->clear(&ret);
		setItemNum(i);
	}

	void setDemo(FontDemo* demo) {
		this->demo = demo;
	}
};

static void okFunc(Widget* wd) {
	FontDialog* dialog = (FontDialog*)wd->getWin();
	dialog->submit(Dialog::RES_OK);
}

static void cancelFunc(Widget* wd) {
	FontDialog* dialog = (FontDialog*)wd->getWin();
	dialog->submit(Dialog::RES_CANCEL);
}

void FontDialog::onBuild() {
	RootWidget* root = new RootWidget();
	setRoot(root);
	root->setType(Container::VERTICLE);
	

	Container* c = new Container();
	c->setType(Container::HORIZONTAL);
	root->add(c);

	FontList* list = new FontList(this);
	list->loadFont();
	list->setItemSize(20);
	list->fix(120, 0);
	c->add(list);
	root->focus(list);

	Scroller* scrollerV = new Scroller();
	scrollerV->fix(8, 0);
	c->add(scrollerV);
	list->setScrollerV(scrollerV);

	Split* split = new Split();
	split->attach(list);
	c->add(split);

	FontDemo* demo = new FontDemo();
	c->add(demo);
	list->setDemo(demo);

	scrollerV = new Scroller();
	scrollerV->fix(8, 0);
	c->add(scrollerV);
	demo->setScrollerV(scrollerV);

	c = new Container();
	c->setType(Container::HORIZONTAL);
	c->fix(0, 20);
	root->add(c);

	LabelButton* okButton = new LabelButton("OK");
	okButton->onClickFunc = okFunc;
	c->add(okButton);

	LabelButton* cancelButton = new LabelButton("Cancel");
	cancelButton->onClickFunc = cancelFunc;
	c->add(cancelButton);

	list->select(0);
}

FontDialog::FontDialog() {
}

string FontDialog::getResult() {
	return fontName; 
}
