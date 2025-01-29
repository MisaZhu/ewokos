#ifndef WIDGET_EDIT_LINE_HH
#define WIDGET_EDIT_LINE_HH

#include <Widget/Widget.h>
#include <string>

using namespace EwokSTL;
namespace Ewok {

class EditLine: public Widget {
protected:
	uint32_t curTimerCounter;
	bool showCur;
	string content;
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r);
	bool onIM(xevent_t* ev);

	virtual void onInput();
	void onTimer(uint32_t timerFPS, uint32_t timerStep);
public:
	void (*onInputFunc)(Widget* wd);

	const string& getContent() { return content; }
	void setContent(const string& content);

	EditLine();
};

}

#endif
