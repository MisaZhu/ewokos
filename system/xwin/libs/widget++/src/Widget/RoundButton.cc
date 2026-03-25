#include <Widget/RoundButton.h>

namespace Ewok {

void RoundButton::paintPanel(graph_t* g, XTheme* theme, const grect_t& rect) {
	graph_fill_round(g, rect.x+roundWidth, rect.y+roundWidth,
			rect.w-2*roundWidth, rect.h-2*roundWidth, round-roundWidth, theme->basic.bgColor);
}

void RoundButton::paintDown(graph_t* g, XTheme* theme, const grect_t& rect) {
	paintPanel(g, theme, rect);
	graph_round_3d(g, rect.x, rect.y, rect.w, rect.h, round, roundWidth, theme->basic.bgColor, true);
}

void RoundButton::paintUp(graph_t* g, XTheme* theme, const grect_t& rect) {
	paintPanel(g, theme, rect);
	graph_round_3d(g, rect.x, rect.y, rect.w, rect.h, round, roundWidth, theme->basic.bgColor, false);
}

void RoundButton::paintDisabled(graph_t* g, XTheme* theme, const grect_t& rect) {
	graph_fill_round_3d(g, rect.x, rect.y, rect.w, rect.h, round, roundWidth, theme->basic.bgDisableColor, false);
}

void RoundButton::setRound(uint32_t r, uint32_t rw) {
	round = r;
	roundWidth = rw;

	if(round < 3)
		round = 3;
	if(roundWidth > (r/2))
		roundWidth = r/2;
}

void RoundButton::setAttr(const string& attr, json_var_t*value) {
	Button::setAttr(attr, value);
	if(attr == "round") {
		setRound(json_var_get_int(value), roundWidth);
	}
	else if(attr == "roundWidth") {
		setRound(round, json_var_get_int(value));
	}
}

RoundButton::RoundButton() {
	setRound(10, 1);
}

}