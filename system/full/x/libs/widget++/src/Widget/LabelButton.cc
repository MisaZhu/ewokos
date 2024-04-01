#include <Widget/LabelButton.h>

namespace Ewok {

void LabelButton::paintDown(graph_t* g, XTheme* theme, const grect_t& r) {
	Button::paintDown(g, theme, r);

	if(theme->getFont() == NULL)
		return;
	graph_draw_text_font_align(g, r.x, r.y, r.w, r.h,
			label.c_str(), theme->getFont(), theme->basic.fgColor, FONT_ALIGN_CENTER);
}

void LabelButton::paintUp(graph_t* g, XTheme* theme, const grect_t& r) {
	Button::paintUp(g, theme, r);
	if(theme->getFont() == NULL)
		return;
	graph_draw_text_font_align(g, r.x, r.y, r.w, r.h,
			label.c_str(), theme->getFont(), theme->basic.fgColor, FONT_ALIGN_CENTER);
}

void LabelButton::paintDisabled(graph_t* g, XTheme* theme, const grect_t& r) {
	Button::paintDisabled(g, theme, r);
	if(theme->getFont() == NULL)
		return;
	graph_draw_text_font_align(g, r.x, r.y, r.w, r.h,
			label.c_str(), theme->getFont(), 0xffdddddd, FONT_ALIGN_CENTER);
}

void LabelButton::setLabel(const string& label) {
	this->label = label;
	update();
}

}