#ifndef WIDGET_POPUP_HH
#define WIDGET_POPUP_HH

#include <Widget/WidgetWin.h>

namespace Ewok {

class Popup: public WidgetWin {
protected:
    void onUnfocus();
public:
    virtual void hide();
    bool popup(XWin* owner, int x, int y, uint32_t w, uint32_t h, const char* title, uint32_t style);
};



}

#endif