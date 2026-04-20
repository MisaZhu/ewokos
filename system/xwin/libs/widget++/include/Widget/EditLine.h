#ifndef WIDGET_EDIT_LINE_HH
#define WIDGET_EDIT_LINE_HH

#include <Widget/Widget.h>
#include <Widget/WidgetWin.h>
#include <string>

using namespace std;
namespace Ewok {

typedef void (*InputFuncT)(Widget* wd, uint32_t input, void* arg);

class EditLine: public Widget {
	bool showXIM;
protected:
	void* onInputFuncArg;
	InputFuncT onInputFunc;

	uint32_t curTimerCounter;
	bool showCur;
	string content;
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r);
	bool onIM(xevent_t* ev);

	virtual void onInput(uint32_t key);
	void onTimer(uint32_t timerFPS, uint32_t timerStep);
	bool onMouse(xevent_t* ev);
public:
	void setOnInputFunc(InputFuncT func, void* arg);

	const string& getContent() { return content; }
	void setContent(const string& content);

	EditLine();
};

}

#endif
