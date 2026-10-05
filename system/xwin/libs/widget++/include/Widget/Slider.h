#ifndef WIDGET_SLIDER_HH
#define WIDGET_SLIDER_HH

#include <Widget/Widget.h>

namespace Ewok {

class Scrollable;

class Slider: public Widget {
    Scrollable* widget;
protected:
    uint32_t range;
    uint32_t pos;
    bool horizontal;
    bool isDragging; // marks whether it is being dragged
    int lastMousePos; // records the last mouse position
    int lastMouseOffset; // records the last mouse position

    void onRepaint(graph_t* g, XTheme* theme, const grect_t& r);
    virtual void drawBG(graph_t* g, XTheme* theme, const grect_t& r);
    virtual void drawPos(graph_t* g, XTheme* theme, const grect_t& r);
    bool onMouse(xevent_t* ev); // handle mouse events
    virtual void onPosChange() {};

public:
    Slider(bool h = true);

    void setScrollable(Scrollable* widget);
    void setRange(uint32_t range);
    void setPos(uint32_t pos);
    void setValue(uint32_t pos);
    uint32_t getPos() { return pos; }
    uint32_t getValue();
    uint32_t getRange() { return range; };
};

}

#endif