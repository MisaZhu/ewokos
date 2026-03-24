#ifndef WIDGET_ROUND_BUTTON_HH
#define WIDGET_ROUND_BUTTON_HH

#include <Widget/Button.h>

namespace Ewok {

class RoundButton: public Button {
protected:
	uint32_t round;
	uint32_t roundWidth;
	virtual void paintPanel(graph_t* g, XTheme* theme, const grect_t& r);
	virtual void paintDown(graph_t* g, XTheme* theme, const grect_t& r);
	virtual void paintUp(graph_t* g, XTheme* theme, const grect_t& r);
	virtual void paintDisabled(graph_t* g, XTheme* theme, const grect_t& r);
	void setAttr(const string& attr, json_var_t*value);

public:
	void setRound(uint32_t round, uint32_t roundWdith);
	RoundButton();
};

}

#endif
