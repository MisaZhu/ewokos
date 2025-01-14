#ifndef WIDGET_TEXT_HH
#define WIDGET_TEXT_HH

#include <Widget/Scrollable.h>

namespace Ewok {

class Text: public Scrollable {
	font_t* font;
	uint32_t fontSize;

	void scrollBack();
	void scrollForward();
protected:
    uint16_t *content;
	int offset;
    int contentSize;
	int pageSize;

	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r);

	bool onScroll(int step, bool horizontal);

	void updateScroller();

	bool onMouse(xevent_t* ev);
	bool onIM(xevent_t* ev);

public: 
	Text();
	~Text();

    void setContent(const char* ctnt, uint32_t size);

    inline int getOffset() { return offset; }
    inline int getConttentSize() { return contentSize; }
	inline uint32_t getFontSize() { return fontSize; }

	void setFont(const string& name);
	void setFontSize(uint32_t fontSize);
};

}
#endif