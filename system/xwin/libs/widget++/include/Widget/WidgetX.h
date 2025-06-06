#ifndef WIDGET_X_HH
#define WIDGET_X_HH

#include <Widget/WidgetWin.h>

namespace Ewok {

void widgetRegWin(WidgetWin* win);
void widgetUnregWin(WidgetWin* win);
void widgetXRun(X* x, WidgetWin* win);

}

#endif
