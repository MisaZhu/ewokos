#ifndef WIDGET_BLANK_HH
#define WIDGET_BLANK_HH

#include <Widget/Widget.h>

namespace Ewok {

class Blank: public Widget {
protected:
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r);
public:
	Blank();
};

}

#endif
