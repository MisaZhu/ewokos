#include <Widget/WidgetX.h>
#include <x++/X.h>
#include <ewoksys/proc.h>
#include <ewoksys/kernel_tic.h>
#include <vector>

namespace Ewok {

#define MAX_WIN 32
static WidgetWin* _winList[MAX_WIN] = {0};

void widgetRegWin(WidgetWin* win) {
    for(int i=0; i<MAX_WIN; i++) {
        if(_winList[i] == NULL) {
            _winList[i] = win;
            return;
        }
    }
}

void widgetUnregWin(WidgetWin* win) {
    for(int i=0; i<MAX_WIN; i++) {
        if(_winList[i] == win) {
            _winList[i] = NULL;
            return;
        }
    }
}

static void loop(void* p) {
    uint64_t tik = kernel_tic_ms(0);
    uint32_t tm = 10;// 100 fps(10ms)

    for(int i=0; i<MAX_WIN; i++) {
        WidgetWin* win = (WidgetWin*)_winList[i];
        if(win == NULL)
            continue;
        RootWidget* root = win->getRoot();
        if(root != NULL) {
            win->doTimer();
            root->repaintWin();
        }
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