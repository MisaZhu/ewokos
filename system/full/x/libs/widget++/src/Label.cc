#include <Widget/Label.h>

using namespace EwokSTL;
namespace Ewok {

void Label::onRepaint(graph_t* g) {
	Widget::onRepaint(g);
	if(font == NULL)
		return;

	grect_t rect = getRootArea();
	graph_draw_text_font_align(g, rect.x+marginH, rect.y+marginV, rect.w-marginH*2, rect.h-marginV*2,
				label.c_str(), font, fgColor, FONT_ALIGN_CENTER);
}

Label::Label(font_t* font, const char* str) {
	this->font = font;
	label = str;
	alpha = true;
}

Label::~Label(void) {
}

void Label::setLabel(const char* str) {
	label = str;
	update();
}

gsize_t Label::getMinSize(void) {
	gsize_t sz = { 0, 0 };
	if(font != NULL)
		font_text_size(label.c_str(), font, (uint32_t*)&sz.w, (uint32_t*)&sz.h);
	return sz;
}

}