#include <Widget/Button.h>

namespace Ewok {

bool Button::onMouse(xevent_t* ev) {
	if(ev->state == XEVT_MOUSE_DOWN) {
		state = STATE_DOWN;
		update();
	}
	else if(ev->state == XEVT_MOUSE_UP) {
		state = STATE_UP;
		update();
	}
	else if(ev->state == XEVT_MOUSE_CLICK) {
		onClick();
	}
	return true;
}

void Button::paintDown(graph_t* g, XTheme* theme, const grect_t& rect) {
	graph_fill(g, rect.x, rect.y, rect.w, rect.h, theme->basic.bgColor);
	uint32_t d, b;
	graph_get_3d_color(theme->basic.bgColor, &d, &b);
	graph_box_3d(g, rect.x, rect.y, rect.w, rect.h, d, b);
}

void Button::paintUp(graph_t* g, XTheme* theme, const grect_t& rect) {
	graph_fill(g, rect.x, rect.y, rect.w, rect.h, theme->basic.bgColor);
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
	onClickFunc = NULL;
}

}