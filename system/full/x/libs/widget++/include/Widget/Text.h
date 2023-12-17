#ifndef WIDGET_TEXT_HH
#define WIDGET_TEXT_HH

#include <Widget/Widget.h>
#include <textview/textview.h>
#include <string>

using namespace EwokSTL;
namespace Ewok {

class Text: public Widget {
	textview_t textview;
	string text;
	graph_t* bufferGraph;
	bool reset;
	bool resetText;
protected:
	void onRepaint(graph_t* g, const Theme* theme, const grect_t& r);

public:
	Text(const string& str);
	~Text(void);

	void setText(const string& str);
	void onFont();
};

}

#endif
