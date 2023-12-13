#include <Widget/Label.h>

using namespace EwokSTL;
namespace Ewok {

void Label::onRepaint(graph_t* g, const Theme* theme, const grect_t& r) {
	graph_fill(g, r.x, r.y, r.w, r.h, theme->bgColor);
	graph_draw_text_font_align(g, r.x+marginH, r.y+marginV, r.w-marginH*2, r.h-marginV*2,
				label.c_str(), font, theme->fgColor, FONT_ALIGN_CENTER);
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
	sz.w += marginH*2;
	sz.h += marginV*2;
	return sz;
}

}