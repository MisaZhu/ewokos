#include <Widget/WidgetX.h>
#include <x++/X.h>
#include <ewoksys/proc.h>

namespace Ewok {



static void loop(void* p) {
    WidgetWin* win = (WidgetWin*)p;
    if(win == NULL)
        return;
	if(!win->isPainting()) {
        RootWidget* root = win->getRoot();
        if(root != NULL) {
    		root->repaintWin();
        }
    }
    proc_usleep(10000);
}

void widgetXRun(X* x, WidgetWin* win) {
    x->run(loop, win);
}

}