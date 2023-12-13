#include <Widget/LabelButton.h>

namespace Ewok {

void LabelButton::paintDown(graph_t* g, const Theme* theme, const grect_t& r) {
	Button::paintDown(g, theme, r);
	graph_draw_text_font_align(g, r.x, r.y, r.w, r.h,
			label.c_str(), theme->font, theme->fgColor, FONT_ALIGN_CENTER);
}

void LabelButton::paintUp(graph_t* g, const Theme* theme, const grect_t& r) {
	Button::paintUp(g, theme, r);
	graph_draw_text_font_align(g, r.x, r.y, r.w, r.h,
			label.c_str(), theme->font, theme->fgColor, FONT_ALIGN_CENTER);
}

void LabelButton::paintDisabled(graph_t* g, const Theme* theme, const grect_t& r) {
	Button::paintDisabled(g, theme, r);
	graph_draw_text_font_align(g, r.x, r.y, r.w, r.h,
			label.c_str(), theme->font, 0xffdddddd, FONT_ALIGN_CENTER);
}

void LabelButton::setLabel(const string& label) {
	this->label = label;
	update();
}

}