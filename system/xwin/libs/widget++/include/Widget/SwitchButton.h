#ifndef WIDGET_SWITCH_BUTTON_HH
#define WIDGET_SWITCH_BUTTON_HH

#include <Widget/Button.h>

namespace Ewok {

class SwitchButton;
typedef void (*SwitchedFuncT)(SwitchButton* wd, bool on, void* arg);

class SwitchButton: public Button {
protected:
	bool on;
	uint32_t onColor;
	uint32_t roundWidth;

	void paintTrack(graph_t* g, XTheme* theme, const grect_t& r, uint32_t trackColor);
	void paintKnob(graph_t* g, XTheme* theme, const grect_t& r, uint32_t knobColor, bool pressed);
	virtual void paintDown(graph_t* g, XTheme* theme, const grect_t& r);
	virtual void paintUp(graph_t* g, XTheme* theme, const grect_t& r);
	virtual void paintDisabled(graph_t* g, XTheme* theme, const grect_t& r);
	virtual bool onMouse(xevent_t* ev);
	virtual void onSwitch(bool on) { }
	void setAttr(const string& attr, json_var_t* value);

	SwitchedFuncT onSwitchFunc;
	void* onSwitchFuncArg;
public:
	SwitchButton();

	void setOn(bool on);
	inline bool isOn() { return on; }
	inline void setOnColor(uint32_t color) { onColor = color; update(); }
	void setSwitchFunc(SwitchedFuncT func, void* arg = NULL);
};

}

#endif
