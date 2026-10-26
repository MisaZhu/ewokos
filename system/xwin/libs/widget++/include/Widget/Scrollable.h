#ifndef WIDGET_SCROLLABLE_HH
#define WIDGET_SCROLLABLE_HH

#include <Widget/Scroller.h>

namespace Ewok {

class Scrollable : public Widget {
protected:
    Scroller* scrollerV;
    string    scrollerVName;
    Scroller* scrollerH;
    string    scrollerHName;

	gpos_t  last_mouse_down;

    uint8_t defaultScrollType;
    uint16_t dragStep;

    void onResize();

	virtual bool onScroll(int step, bool horizontal) = 0;
    virtual void updateScroller() = 0;

    /* smooth pixel-based scrolling: dx/dy are raw mouse movement in pixels.
     * Widgets that support 1:1 pixel scrolling (List, Grid) override this and
     * return true to consume the delta; the default returns false so the
     * legacy item-step dragging in onMouse() is used instead. */
    virtual bool onScrollPixels(int dx, int dy);

    bool onMouse(xevent_t* ev);

    void setScrollerInfo(uint32_t range, uint32_t pos, uint32_t scrollW, bool horizontal);
	void setAttr(const string& attr, json_var_t*value);
public:
    static const uint8_t SCROLL_TYPE_V = 0;
    static const uint8_t SCROLL_TYPE_H = 1;

	void scroll(int step, bool horizontal);
    void setScrollerV(Scroller* r);
    void setScrollerH(Scroller* r);
    void setDefaultScrollType(uint8_t type);
    void setDragStep(uint16_t step);

    Scrollable();
};

}

#endif
