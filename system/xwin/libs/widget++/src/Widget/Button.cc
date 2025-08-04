#include <Widget/Button.h>

namespace Ewok {

bool Button::onMouse(xevent_t* ev) {
	if(ev->state == MOUSE_STATE_DOWN) {
		state = STATE_DOWN;
		update();
		return true;
	}
	else if(ev->state == MOUSE_STATE_UP) {
		state = STATE_UP;
		update();
		return true;
	}
	else if(ev->state == MOUSE_STATE_CLICK) {
		return true;
	}
	return false;
}

void Button::paintPanel(graph_t* g, XTheme* theme, const grect_t& rect) {
	graph_fill(g, rect.x, rect.y, rect.w, rect.h, theme->basic.bgColor);
}

void Button::paintDown(graph_t* g, XTheme* theme, const grect_t& rect) {
	paintPanel(g, theme, rect);
	uint32_t d, b;
	graph_get_3d_color(theme->basic.bgColor, &d, &b);
	graph_box_3d(g, rect.x, rect.y, rect.w, rect.h, d, b);
}

void Button::paintUp(graph_t* g, XTheme* theme, const grect_t& rect) {
	paintPanel(g, theme, rect);
	uint32_t d, b;
	graph_get_3d_color(theme->basic.bgColor, &d, &b);
	graph_box_3d(g, rect.x, rect.y, rect.w, rect.h, b, d);
}

void Button::paintDisabled(graph_t* g, XTheme* theme, const grect_t& rect) {
	graph_fill_3d(g, rect.x, rect.y, rect.w, rect.h, theme->basic.bgDisableColor, false);
}

void Button::onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
	if(disabled) {
		paintDisabled(g, theme, r);
		return;
	}

	if(state == STATE_DOWN)
		paintDown(g, theme, r);
	else
		paintUp(g, theme, r);
}

Button::Button() {
	state = STATE_UP;
}

}