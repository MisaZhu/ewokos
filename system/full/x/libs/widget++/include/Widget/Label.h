#ifndef WIDGET_LABEL_HH
#define WIDGET_LABEL_HH

#include <Widget/Widget.h>
#include <upng/upng.h>
#include <string>
#include <font/font.h>

using namespace EwokSTL;
namespace Ewok {

class Label: public Widget {
	font_t* font;
	string label;
protected:
	void onRepaint(graph_t* g, grect_t* rect) {
		Widget::onRepaint(g, rect);
		if(font == NULL)
			return;
		graph_draw_text_font_align(g, rect->x+marginH, rect->y+marginV, rect->w-marginH*2, rect->h-marginV*2,
					label.c_str(), font, fgColor, FONT_ALIGN_CENTER);
	}

public:
	Label(font_t* font, const char* str) {
		this->font = font;
		label = str;
	}

	~Label(void) {
	}

	void setLabel(const char* str) {
		label = str;
		update();
	}

	gsize_t getMinSize(void) {
		gsize_t sz = { 0, 0 };
		if(font != NULL)
			font_text_size(label.c_str(), font, (uint32_t*)&sz.w, (uint32_t*)&sz.h);
		return sz;
	}
};

}

#endif