#ifndef WIDGET_LABEL_HH
#define WIDGET_LABEL_HH

#include <Widget/Widget.h>
#include <string>
#include <font/font.h>

using namespace EwokSTL;
namespace Ewok {

class Label: public Widget {
	font_t* font;
public:
	string label;
protected:
	void onRepaint(graph_t* g, const grect_t& r);

public:
	Label(font_t* font, const char* str);
	~Label(void);

	void setLabel(const char* str);
	gsize_t getMinSize(void);
};

}

#endif
