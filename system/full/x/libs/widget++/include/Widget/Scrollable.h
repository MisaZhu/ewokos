#ifndef WIDGET_SCROLLABLE_HH
#define WIDGET_SCROLLABLE_HH

#include <Widget/Scroller.h>

namespace Ewok {

class Scrollable : public Widget {
protected:
    Scroller* scrollerV;
    Scroller* scrollerH;

	virtual void onScroll(int step, bool horizontal) = 0;
    virtual void updateScroller() = 0;

    void setScrollerInfo(uint32_t range, uint32_t pos, uint32_t scrollW, bool horizontal);
public:
	void scroll(int step, bool horizontal);
    void setScrollerV(Scroller* r);
    void setScrollerH(Scroller* r);

    Scrollable() { scrollerV = NULL; scrollerH = NULL; }
};

}

#endif
