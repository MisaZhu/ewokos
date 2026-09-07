#include <Widget/SwitchButton.h>

namespace Ewok {

void SwitchButton::paintTrack(graph_t* g, XTheme* theme, const grect_t& rect, uint32_t trackColor) {
	int32_t round = rect.h/2;
	graph_fill_round(g, rect.x, rect.y, rect.w, rect.h, round, trackColor);
	graph_round_3d(g, rect.x, rect.y, rect.w, rect.h, round, roundWidth, theme->basic.bgColor, true);
}

void SwitchButton::paintKnob(graph_t* g, XTheme* theme, const grect_t& rect, uint32_t knobColor, bool pressed) {
	int32_t r = rect.h/2;
	int32_t cx = on ? (rect.x + rect.w - r) : (rect.x + r);
	int32_t cy = rect.y + r;
	graph_fill_circle_3d(g, cx, cy, r-roundWidth, roundWidth, knobColor, pressed);
}

void SwitchButton::paintDown(graph_t* g, XTheme* theme, const grect_t& rect) {
	paintTrack(g, theme, rect, on ? onColor : theme->basic.bgColor);
	paintKnob(g, theme, rect, theme->basic.bgColor, true);
}

void SwitchButton::paintUp(graph_t* g, XTheme* theme, const grect_t& rect) {
	paintTrack(g, theme, rect, on ? onColor : theme->basic.bgColor);
	paintKnob(g, theme, rect, theme->basic.bgColor, false);
}

void SwitchButton::paintDisabled(graph_t* g, XTheme* theme, const grect_t& rect) {
	paintTrack(g, theme, rect, theme->basic.bgDisableColor);
	paintKnob(g, theme, rect, theme->basic.bgDisableColor, false);
}

bool SwitchButton::onMouse(xevent_t* ev) {
	if(ev->state == MOUSE_STATE_CLICK) {
		setOn(!on);
		return true;
	}
	return Button::onMouse(ev);
}

void SwitchButton::setOn(bool isOn) {
	if(on == isOn)
		return;
	on = isOn;
	onSwitch(on);
	if(onSwitchFunc != NULL)
		onSwitchFunc(this, on, onSwitchFuncArg);
	update();
}

void SwitchButton::setSwitchFunc(SwitchedFuncT func, void* arg) {
	onSwitchFunc = func;
	onSwitchFuncArg = arg;
}

void SwitchButton::setAttr(const string& attr, json_var_t*value) {
	Button::setAttr(attr, value);
	if(attr == "on") {
		setOn(json_var_get_int(value) != 0);
	}
	else if(attr == "onColor") {
		setOnColor((uint32_t)json_var_get_int(value));
	}
	else if(attr == "roundWidth") {
		roundWidth = json_var_get_int(value);
	}
}

SwitchButton::SwitchButton() {
	on = false;
	onColor = 0xff00aa00;
	roundWidth = 2;
	onSwitchFunc = NULL;
	onSwitchFuncArg = NULL;
}

}
