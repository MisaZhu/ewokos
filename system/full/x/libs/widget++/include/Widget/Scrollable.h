#ifndef WIDGET_SCROLLABLE_HH
#define WIDGET_SCROLLABLE_HH

#include <Widget/Scroller.h>

namespace Ewok {

class Scrollable : public Widget {
protected:
    Scroller* scrollerV;
    Scroller* scrollerH;
	gpos_t  last_mouse_down;

    uint8_t defaultScrollType;
    uint16_t dragStep;

    void onResize();

	virtual bool onScroll(int step, bool horizontal) = 0;
    virtual void updateScroller() = 0;

    bool onMouse(xevent_t* ev);

    void setScrollerInfo(uint32_t range, uint32_t pos, uint32_t scrollW, bool horizontal);
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
