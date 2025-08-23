#ifndef WIDGET_LABEL_BUTTON_HH
#define WIDGET_LABEL_BUTTON_HH

#include <Widget/Button.h>
#include <string>

using namespace std;
namespace Ewok {

class LabelButton: public Button {
protected:
	string label;
	virtual void paintDown(graph_t* g, XTheme* theme, const grect_t& r);
	virtual void paintUp(graph_t* g, XTheme* theme, const grect_t& r);
	virtual void paintDisabled(graph_t* g, XTheme* theme, const grect_t& r);
	void setAttr(const string& attr, json_var_t*value);

public:
	LabelButton(const string& label = "") { this->label = label; }

	inline const string& getLabel(void) {
		return label;
	}

	void setLabel(const string& label);
};

}

#endif
