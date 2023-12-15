#ifndef WIDGET_LABEL_HH
#define WIDGET_LABEL_HH

#include <Widget/Widget.h>
#include <Widget/FontUnit.h>
#include <string>

using namespace EwokSTL;
namespace Ewok {

class Label: public Widget, public FontUnit {
	string label;
protected:
	void onRepaint(graph_t* g, const Theme* theme, const grect_t& r);

public:
	Label(const string& str);
	~Label(void);

	void setLabel(const string& str);
};

}

#endif
