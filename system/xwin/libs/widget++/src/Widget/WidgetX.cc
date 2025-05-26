#include <Widget/WidgetX.h>
#include <x++/X.h>
#include <ewoksys/proc.h>
#include <ewoksys/kernel_tic.h>

namespace Ewok {



static void loop(void* p) {
    WidgetWin* win = (WidgetWin*)p;
    if(win == NULL)
        return;
    RootWidget* root = win->getRoot();

    uint64_t tik = kernel_tic_ms(0);
	uint32_t tm = 10;// 100 fps(10ms)

    if(root != NULL) {
        win->doTimer();
        root->repaintWin();
    }

	uint32_t gap = (uint32_t)(kernel_tic_ms(0) - tik);
	if(gap < tm) {
		gap = tm - gap;
		proc_usleep(gap*1000);
	}
}

void widgetXRun(X* x, WidgetWin* win) {
    x->run(loop, win);
}

}