#ifndef WIDGET_EDIT_LINE_HH
#define WIDGET_EDIT_LINE_HH

#include <Widget/Widget.h>
#include <string>

using namespace EwokSTL;
namespace Ewok {

class EditLine: public Widget {
protected:
	string content;
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r);
	bool onIM(xevent_t* ev);

	virtual void onInput();
public:
	void (*onInputFunc)(Widget* wd);

	const string& getContent() { return content; }
	void setContent(const string& content);
};

}

#endif
