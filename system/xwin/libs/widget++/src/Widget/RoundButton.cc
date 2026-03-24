#include <Widget/RoundButton.h>

namespace Ewok {

void RoundButton::paintPanel(graph_t* g, XTheme* theme, const grect_t& rect) {
	graph_fill_round(g, rect.x, rect.y, rect.w, rect.h, round, theme->basic.bgColor);
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

RoundButton::RoundButton() {
	round = 6;
	roundWidth = 1;
}

}