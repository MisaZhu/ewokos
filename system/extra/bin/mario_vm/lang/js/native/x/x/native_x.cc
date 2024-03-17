#include "native_x.h"
#include <x++/X.h>
#include <x++/XWin.h>
#include <ewoksys/klog.h>

using namespace Ewok;


static void free_none(void* p) {
	return;
}

class JSWin : public XWin {
public:
	var_t* var_win;
	vm_t * vm;
protected:
	void onRepaint(graph_t* g) { 
		var_t* arg_g = var_new_obj(vm, g, free_none);

		var_t* args = var_new(vm);
		var_add(args, "g", arg_g);

		call_m_func_by_name(vm, var_win, "onRepaint", args);
		var_unref(args);
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
	JSWin *xwin = new JSWin();
	x->open(0, xwin, 10, 10, 300, 200, "xwin", XWIN_STYLE_NORMAL);

	var_t* var_win = var_new_obj(vm, xwin, destroy_win);
	xwin->vm = vm;
	xwin->var_win = var_win;

	xwin->setVisible(true);
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
