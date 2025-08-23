#ifndef WIDGET_SPLIT_HH
#define WIDGET_SPLIT_HH

#include <Widget/Widget.h>

namespace Ewok {

class Split: public Widget {
protected:
	bool horizontal;
	uint32_t width;
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r);
	void setAttr(const string& attr, json_var_t*value);
	void onAdd();
public:
	Split();
	void setWidth(uint32_t w);
};

}

#endif
