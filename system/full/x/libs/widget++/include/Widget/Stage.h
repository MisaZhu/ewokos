#ifndef WIDGET_STAGE_HH
#define WIDGET_STAGE_HH

#include <x++/XWin.h>
#include <Widget/Container.h>

namespace Ewok {

class Stage: public Container {
protected:
    Widget* activeWidget;
public:
	Stage();

    void active(Widget* wd);
};

}

#endif