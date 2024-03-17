#include "native_x.h"
#include <x++/X.h>
#include <x++/XWin.h>
#include <ewoksys/klog.h>

using namespace Ewok;

class JSWin : public XWin {
protected:
	void onRepaint(graph_t* g) { 
	}
};


static X _x;
static X* get_x(void) {
	return &_x;
}

static void destroy_win(void* p) {
	XWin* xwin = (XWin*)p;
	xwin->close();
	delete xwin;
}

/** String */
var_t* native_x_open(vm_t* vm, var_t* env, void* data) {
	X* x = get_x();
	XWin *xwin = new JSWin();
	x->open(0, xwin, 10, 10, 300, 200, "xwin", XWIN_STYLE_NORMAL);
	xwin->setVisible(true);

	var_t* var_win = var_new_obj(vm, xwin, destroy_win);
	return var_win;
}

var_t* native_x_run(vm_t* vm, var_t* env, void* data) {
	X* x = get_x();
	x->run(NULL);
	return NULL;
}

#define CLS_X "X"

#ifdef __cplusplus /* __cplusplus */
extern "C" {
#endif

void reg_native_x(vm_t* vm) {
	var_t* cls = vm_new_class(vm, CLS_X);
	vm_reg_static(vm, cls, "open()", native_x_open, NULL); 
	vm_reg_static(vm, cls, "run()", native_x_run, NULL); 
}

#ifdef __cplusplus /* __cplusplus */
}
#endif
