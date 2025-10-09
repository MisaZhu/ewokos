#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <Widget/Image.h>
#include <Widget/Label.h>
#include <Widget/LabelButton.h>
#include <Widget/List.h>
#include <Widget/EditLine.h>
#include <Widget/Grid.h>
#include <Widget/Scroller.h>
#include <Widget/Splitter.h>
#include <WidgetEx/FileDialog.h>
#include <WidgetEx/ConfirmDialog.h>

#include <x++/X.h>
#include <unistd.h>
#include <font/font.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/kernel_tic.h>
#include <graph/graph_png.h>
#include <openlibm.h>

using namespace Ewok;

class MyList: public List {
protected:
	void drawItem(graph_t* g, XTheme* theme, int32_t index, const grect_t& r) {
		if(index == itemSelected)
			graph_box(g, r.x, r.y, r.w, r.h, 0xffff0000);
		else
			graph_box(g, r.x, r.y, r.w, r.h, 0xffaaaaaa);
		char s[8];
		snprintf(s, 7, "%d", index);
		graph_draw_text_font(g, r.x+2, r.y+2, s, theme->getFont(), theme->basic.fontSize, 0xff000000);
	}

	void onSelect(int index) {
		slog("index: %d\n", index);
	}

public:
	MyList() {
	}
};

class Anim: public Widget {
	graph_t* img;
	uint32_t step;
	uint32_t pos;
	int32_t steps;
	EditLine* editLine;
protected:
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		if(img == NULL)
			return;
		graph_gradation(g, r.x, r.y, r.w, r.h, 0xffffffff, 0xff00ff00, true);
		graph_blt_alpha(img, step*(img->w/steps), 0, img->w/steps, img->h,
				g, r.x+pos, r.y+(r.h-img->h)/2, img->w/steps, img->h, 0xff);

		graph_draw_text_font(g, r.x+pos+img->w/steps, r.y+2, editLine->getContent().c_str(), theme->getFont(), theme->basic.fontSize, theme->basic.fgColor);

		if(pos > r.w)
			pos = 0;
	}

	void onTimer(uint32_t timerFPS, uint32_t timerStep) {
		step++;
		if (step >= steps)
			step = 0;
		pos += img->w/steps/steps;
		update();
	}

public: 
	Anim(EditLine* editLine) {
		this->editLine = editLine;
		step = 0;
		img = png_image_new(X::getResName("data/walk.png").c_str());
		steps = 8;
		pos = 0;
	}

	~Anim() {
		if(img != NULL)
			graph_free(img);
	}
};


class PaintWidget : public Widget {
    uint32_t i;
    float angle; // 新增角度变量
public:
    inline PaintWidget() {
        i = 0;
        angle = 0.0f; // 初始化角度
    }
    
    inline ~PaintWidget() {
    }

protected:

    void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
        // 移除原有的背景填充代码
        // graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.bgColor);
        
        float rad = angle * 3.1415926f / 180.0f; // 转换为弧度
        float cosAngle = cos(rad);
        float sinAngle = sin(rad);
        for (int y = r.y; y < r.y + r.h; y++) {
            for (int x = r.x; x < r.x + r.w; x++) {
                float dx = x - r.x - r.w / 2.0f;
                float dy = y - r.y - r.h / 2.0f;
                float projected = dx * cosAngle + dy * sinAngle;
                float maxProjected = sqrt(r.w * r.w + r.h * r.h) / 2.0f;
                float ratio = (projected + maxProjected) / (2.0f * maxProjected);
                if (ratio > 1.0f) ratio = 1.0f;
                if (ratio < 0.0f) ratio = 0.0f;
                
                // 定义绿色、蓝色和红色
                uint32_t green = 0xFF00FF00;
                uint32_t blue = 0xFF0000FF;
                uint32_t red = 0xFFFF0000;
                
                uint32_t startColor, endColor;
                if (ratio < 0.5f) {
                    startColor = green;
                    endColor = blue;
                    ratio = ratio * 2;
                } else {
                    startColor = blue;
                    endColor = red;
                    ratio = (ratio - 0.5f) * 2;
                }
                
                uint8_t rValue = (uint8_t)((((startColor >> 16) & 0xFF) * (1 - ratio)) + (((endColor >> 16) & 0xFF) * ratio));
                uint8_t gValue = (uint8_t)((((startColor >> 8) & 0xFF) * (1 - ratio)) + (((endColor >> 8) & 0xFF) * ratio));
                uint8_t bValue = (uint8_t)(((startColor & 0xFF) * (1 - ratio)) + ((endColor & 0xFF) * ratio));
                uint32_t color = 0xFF000000 | (rValue << 16) | (gValue << 8) | bValue;
                graph_pixel(g, x, y, color);
            }
        }
        
        char txt[32] = { 0 };
        snprintf(txt, 31, "%d", i);
        font_t* font = theme->getFont();
        uint32_t w;
        font_text_size(txt, font, 10, &w, NULL);
        graph_draw_text_font(g, r.x + r.w - w, r.y, txt, theme->getFont(), 10, theme->basic.fgColor);
    }

    void onTimer(uint32_t timerFPS, uint32_t timerStep) {
		i++;
        angle += 4.0f; // 每次刷新角度增加 4 度
        if (angle >= 360.0f) {
            angle -= 360.0f; // 确保角度在 0-360 度之间
        }
        update();
    }
};

class MyWidgetWin: public WidgetWin{
protected:
	void onDialoged(XWin* from, int res, void* arg) {
		Widget* w = root->getChild("button");
		LabelButton* button = (LabelButton*)w;
		if(res == Dialog::RES_OK)
			button->setLabel("Confirmed");
		else
			button->setLabel("Canceled");
	}

	ConfirmDialog dialog;
public:
	static void onEventFunc(Widget* wd, xevent_t* evt, void* arg) {
		if(evt->type != XEVT_MOUSE || evt->state != MOUSE_STATE_CLICK)
			return;
		MyWidgetWin* win = (MyWidgetWin*)wd->getWin();
		win->dialog.setMessage("Dialog Test");
		win->dialog.popup(win, 200, 100, "dialog", XWIN_STYLE_NO_TITLE);
	}

	MyWidgetWin() {
	}
};


int main(int argc, char** argv) {
	X x;
	MyWidgetWin win;
	RootWidget* root = new RootWidget();
	win.setRoot(root);
	root->setType(Container::VERTICLE);
	

	EditLine* editLine = new EditLine();
	editLine->fix(0, 30);
	editLine->setContent("hello!");
	root->focus(editLine);
	root->add(editLine);

	Widget* wd = new Anim(editLine);
	wd->fix(0, 100);
	root->add(wd);

	Container* c = new Container();
	c->setType(Container::HORIZONTAL);
	root->add(c);

	LabelButton* button = new LabelButton("Dialog Test");
	button->setName("button");
	button->setEventFunc(win.onEventFunc);
	c->add(button);

	Splitter* splitter = new Splitter();
	splitter->attach(button);
	c->add(splitter);

	wd = new PaintWidget();
	c->add(wd);

	MyList* list = new MyList();
	c->add(list);
	list->setItemNum(100);
	list->setItemSize(20);

	Scroller *sr = new Scroller();
	sr->fix(8, 0);
	list->setScrollerV(sr);
	c->add(sr);

	splitter = new Splitter();
	splitter->attach(c);
	root->add(splitter);

	list = new MyList();
	root->add(list);
	list->setItemNum(100);
	list->setItemSize(20);
	list->setDefaultScrollType(Scrollable::SCROLL_TYPE_H);

	sr = new Scroller(true);
	sr->fix(0, 8);
	list->setScrollerH(sr);
	root->add(sr);

	win.open(&x, -1, -1, -1, 0, 0, "widgetTest", XWIN_STYLE_NORMAL);
	win.setTimer(16);

	widgetXRun(&x, &win);	
	return 0;
}