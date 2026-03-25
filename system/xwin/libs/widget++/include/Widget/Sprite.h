#ifndef SPRITE_HH
#define SPRITE_HH

#include <Widget/WidgetWin.h>

namespace Ewok {

class Sprite : public WidgetWin {
protected:
    void onEvent(xevent_t* ev);
    void onRepaint(graph_t* g);
public:
    Sprite() { }
};

}

#endif
