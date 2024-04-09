#ifndef WIDGET_MENU_HH
#define WIDGET_MENU_HH

#include <Widget/WidgetWin.h>

#include <string>
#include <vector>
using namespace EwokSTL;

namespace Ewok {

class Menubar;
class Menu: public WidgetWin {
protected:
    Menubar* menubar;
    void onUnfocus();
public:
    Menu(Menubar* menubar);
};

}

#endif