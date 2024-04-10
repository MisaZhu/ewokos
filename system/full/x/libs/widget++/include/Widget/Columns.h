#ifndef WIDGET_COLUMNS_HH
#define WIDGET_COLUMNS_HH

#include <Widget/Scrollable.h>

namespace Ewok {

struct Column{
	int width;
	bool fixed;
	string title;
};


class Columns: public Scrollable {
	static const uint32_t MAX_COLS = 32;
	uint32_t paintH;

protected:
	Column cols[MAX_COLS];
	uint32_t colNum;
	uint32_t rowNum;
	uint32_t rowH;
	int off_y;

	virtual void drawItem(graph_t* g, XTheme* theme, int row, int col, const grect_t& r) = 0; 
	virtual void drawRowBG(graph_t* g, XTheme* theme, int row, const grect_t& r) { }

	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r);
	bool onScroll(int step, bool horizontal);
	void updateScroller();
	bool onMouse(xevent_t* ev);
    void onResize();
    void layout();
   
public: 
	Columns();
	void add(const string& title, uint32_t width);
    void setRowSize(uint32_t size);
};

}

#endif