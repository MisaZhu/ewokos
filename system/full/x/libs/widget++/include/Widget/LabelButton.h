#ifndef WIDGET_LABEL_BUTTON_HH
#define WIDGET_LABEL_BUTTON_HH

#include <Widget/Button.h>
#include <Widget/FontUnit.h>
#include <string.h>

namespace Ewok {

class LabelButton: public Button, public FontUnit {
protected:
	string label;
	virtual void paintDown(graph_t* g, const Theme* theme, const grect_t& r);
	virtual void paintUp(graph_t* g, const Theme* theme, const grect_t& r);
	virtual void paintDisabled(graph_t* g, const Theme* theme, const grect_t& r);

public:
	LabelButton(const string& label) { this->label = label; }

	void setLabel(const string& label);
};

}

#endif
