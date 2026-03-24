#include <Widget/RoundLabelButton.h>

namespace Ewok {

void RoundLabelButton::paintDown(graph_t* g, XTheme* theme, const grect_t& r) {
	RoundButton::paintDown(g, theme, r);

	font_t* font = theme->getFont();
	if(font == NULL)
		return;
	graph_draw_text_font_align(g, r.x, r.y, r.w, r.h,
			label.c_str(), font, theme->basic.fontSize, theme->basic.fgColor, FONT_ALIGN_CENTER);
}

void RoundLabelButton::paintUp(graph_t* g, XTheme* theme, const grect_t& r) {
	RoundButton::paintUp(g, theme, r);
	font_t* font = theme->getFont();
	if(font == NULL)
		return;
	graph_draw_text_font_align(g, r.x, r.y, r.w, r.h,
			label.c_str(), font, theme->basic.fontSize, theme->basic.fgColor, FONT_ALIGN_CENTER);
}

void RoundLabelButton::paintDisabled(graph_t* g, XTheme* theme, const grect_t& r) {
	RoundButton::paintDisabled(g, theme, r);
	font_t* font = theme->getFont();
	if(font == NULL)
		return;
	graph_draw_text_font_align(g, r.x, r.y, r.w, r.h,
			label.c_str(), font, theme->basic.fontSize, theme->basic.fgDisableColor, FONT_ALIGN_CENTER);
}

void RoundLabelButton::setLabel(const string& label) {
	this->label = label;
	update();
}

void RoundLabelButton::setAttr(const string& attr, json_var_t*value) {
	RoundButton::setAttr(attr, value);
	if(attr == "label") {
		setLabel(json_var_get_str(value));
	}
}

}