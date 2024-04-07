#include <Widget/LabelButton.h>

namespace Ewok {

void LabelButton::paintDown(graph_t* g, XTheme* theme, const grect_t& r) {
	Button::paintDown(g, theme, r);

	font_t* font = theme->getFont();
	if(font == NULL)
		return;
	graph_draw_text_font_align(g, r.x, r.y, r.w, r.h,
			label.c_str(), font, theme->basic.fontSize, theme->basic.fgColor, FONT_ALIGN_CENTER);
}

void LabelButton::paintUp(graph_t* g, XTheme* theme, const grect_t& r) {
	Button::paintUp(g, theme, r);
	font_t* font = theme->getFont();
	if(font == NULL)
		return;
	graph_draw_text_font_align(g, r.x, r.y, r.w, r.h,
			label.c_str(), font, theme->basic.fontSize, theme->basic.fgColor, FONT_ALIGN_CENTER);
}

void LabelButton::paintDisabled(graph_t* g, XTheme* theme, const grect_t& r) {
	Button::paintDisabled(g, theme, r);
	font_t* font = theme->getFont();
	if(font == NULL)
		return;
	graph_draw_text_font_align(g, r.x, r.y, r.w, r.h,
			label.c_str(), font, theme->basic.fontSize, theme->basic.fgDisableColor, FONT_ALIGN_CENTER);
}

void LabelButton::setLabel(const string& label) {
	this->label = label;
	update();
}

}