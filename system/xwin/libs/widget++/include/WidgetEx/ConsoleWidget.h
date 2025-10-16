#ifndef WIDGET_CONSOLE_HH
#define WIDGET_CONSOLE_HH

#include <gterminal/gterminal.h>
#include <Widget/Widget.h>

namespace Ewok {

class ConsoleWidget : public Widget {
	int32_t mouse_last_y;
public:
	ConsoleWidget();
	~ConsoleWidget();

	bool config(const char* fontName,
		uint32_t fontSize,
		int32_t charSpace,
		int32_t lineSpace,
		uint32_t fgColor,
		uint32_t bgColor,
		uint8_t transparent);

	void push(const char* buf, int size);
	void flash();

	void setFont(const string& fontName);
	void setMaxRows(uint32_t maxRows);

	gterminal_t* getTerminal() {
		return &terminal;
	}

protected:
	gterminal_t terminal;
	uint32_t scrollW;

	void drawBG(graph_t* g, const grect_t& r);	
	void drawScrollbar(graph_t* g, const grect_t& r);
	void onFocus(void);
	void onUnfocus(void);
	void onResize();
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r);
	bool onMouse(xevent_t* ev);
	bool onIM(xevent_t* ev);
    virtual void input(int32_t c) = 0;
};

}

#endif
