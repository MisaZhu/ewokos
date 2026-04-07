#ifndef WIDGET_WIN_HH
#define WIDGET_WIN_HH

#include <x++/XWin.h>
#include <Widget/RootWidget.h>

namespace Ewok {

class WidgetWin: public XWin {
	static const uint32_t TIMER_MIN_FPS = 1;
protected:
	uint32_t timerFPS;
	uint32_t lastTimerTick;
	RootWidget* root;
	void onRepaint(graph_t* g);
	void onResize(void);
	void onEvent(xevent_t* ev);
	void onShow(void);
	void onFocus(void);
	void onUnfocus(void);
	void onMove(void);
	bool onClose();
	virtual void onBuild();
public:
	WidgetWin(void);
	~WidgetWin(void);
	inline RootWidget* getRoot() { return root; }

	inline uint32_t getTimerFPS() { return timerFPS; }

	void build();
	void setRoot(RootWidget* root);
	void setTimer(uint32_t fps);
	void doTimer(uint64_t tick);
};

}

#endif
