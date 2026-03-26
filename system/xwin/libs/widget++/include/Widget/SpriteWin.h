#ifndef SPRITE_WIN_HH
#define SPRITE_WIN_HH

#include <Widget/WidgetWin.h>

namespace Ewok {

class SpriteWin : public WidgetWin {
protected:
    void onEvent(xevent_t* ev);
    void onRepaint(graph_t* g);
public:
    SpriteWin() { }
};

}

#endif
