#ifndef _EWOK_LAYOUTWIN_H
#define _EWOK_LAYOUTWIN_H

#include <Widget/WidgetWin.h>  
#include <WidgetEx/LayoutWidget.h>  

#include <string>  
using namespace std;

namespace Ewok {

class LayoutWin : public WidgetWin {
protected:
    LayoutWidget* layoutWidget;
public:
    LayoutWin();
    ~LayoutWin();

    inline LayoutWidget* getLayoutWidget() { return layoutWidget; }
    bool loadConfig(const string& fname);
};

}

#endif