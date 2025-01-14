#include "native_x.h"
#include <x++/X.h>
#include <x++/XWin.h>
#include <ewoksys/klog.h>

using namespace Ewok;

#define CLS_GRAPH "Graph"
#define CLS_X "X"
#define CLS_XWIN "XWin"

static void free_none(void* p) {
	return;
}

class JSWin : public XWin {
public:
	var_t* var_win;
	vm_t * vm;
private:
	void onMouse(xevent_t* xev) {
		var_t* arg = var_new_obj(vm, NULL, NULL);
		var_add(arg, "state", var_new_int(vm, xev->state));
		var_add(arg, "global_x", var_new_int(vm, xev->value.mouse.x));
		var_add(arg, "global_y", var_new_int(vm, xev->value.mouse.y));
		var_add(arg, "rx", var_new_int(vm, xev->value.mouse.rx));
		var_add(arg, "ry", var_new_int(vm, xev->value.mouse.ry));

		gpos_t pos = getInsidePos(xev->value.mouse.x, xev->value.mouse.y);
		var_add(arg, "x", var_new_int(vm, pos.x));
		var_add(arg, "y", var_new_int(vm, pos.y));

		var_t* args = var_new(vm);
		var_add(args, "mouseEvt", arg);
		call_m_func_by_name(vm, var_win, "onMouse", args);
		var_unref(args);
	}
	
	void onIM(xevent_t* xev) {
		var_t* arg = var_new_obj(vm, NULL, NULL);
		var_add(arg, "state", var_new_int(vm, xev->state));
		var_add(arg, "c", var_new_int(vm, xev->value.im.value));

		var_t* args = var_new(vm);
		var_add(args, "imEvt", arg);
		call_m_func_by_name(vm, var_win, "onIM", args);
		var_unref(args);
	}
protected:
	void onRepaint(graph_t* g) { 
		var_t* arg_g = new_obj(vm, CLS_GRAPH, 0);
		arg_g->value = g;
		arg_g->free_func = free_none;
		var_add(arg_g, "width", var_new_int(vm, g->w));
		var_add(arg_g, "height", var_new_int(vm, g->h));

		var_t* args = var_new(vm);
		var_add(args, "g", arg_g);

		call_m_func_by_name(vm, var_win, "onRepaint", args);
		var_unref(args);
	}

	void onEvent(xevent_t* xev) {
		if(xev->type == XEVT_MOUSE) {
			onMouse(xev);
		}
		else if(xev->type == XEVT_IM) {
			onIM(xev);
		}
	}

	void onResize(void) {
		xinfo_t xinfo;
		getInfo(xinfo);

		var_t* arg = var_new_obj(vm, NULL, NULL); 
		var_add(arg, "width", var_new_int(vm, xinfo.wsr.w));
		var_add(arg, "height", var_new_int(vm, xinfo.wsr.h));

		var_t* args = var_new(vm);
		var_add(args, "size", arg);

		call_m_func_by_name(vm, var_win, "onResize", args);
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

//============= x natives =================================
var_t* native_x_open(vm_t* vm, var_t* env, void* data) {
	const char* title = get_str(env, "title");
	int x = get_int(env, "x");
	int y = get_int(env, "y");
	int w = get_int(env, "w");
	int h = get_int(env, "h");

	X* xp = get_x();
	JSWin *xwin = new JSWin();
	xwin->open(xp, 0, x, y, w, h, title, XWIN_STYLE_NORMAL, false);

	var_t* var_win = new_obj(vm, CLS_XWIN, 0);
	var_win->value = xwin;
	var_win->free_func = destroy_win;
	xwin->vm = vm;
	xwin->var_win = var_win;

	return var_win;
}

var_t* native_x_run(vm_t* vm, var_t* env, void* data) {
	X* x = get_x();
	x->run(NULL);
	return NULL;
}

//============= xwin natives =================================

var_t* native_xwin_setVisible(vm_t* vm, var_t* env, void* data) {
	bool visible = get_bool(env, "visible");

	XWin* xwin = (XWin*)get_raw(env, THIS);
	if(xwin == NULL)
		return NULL;

	xwin->setVisible(visible);
	return NULL;
}

var_t* native_xwin_repaint(vm_t* vm, var_t* env, void* data) {
	XWin* xwin = (XWin*)get_raw(env, THIS);
	if(xwin == NULL)
		return NULL;

	xwin->repaint();
	return NULL;
}

var_t* native_xwin_getSize(vm_t* vm, var_t* env, void* data) {
	XWin* xwin = (XWin*)get_raw(env, THIS);
	if(xwin == NULL)
		return NULL;


	xinfo_t xinfo;
	xwin->getInfo(xinfo);
	var_t* ret = var_new_obj(vm, NULL, NULL);
	var_add(ret, "width", var_new_int(vm, xinfo.wsr.w));
	var_add(ret, "height", var_new_int(vm, xinfo.wsr.h));
	return ret;
}

#ifdef __cplusplus /* __cplusplus */
extern "C" {
#endif

void reg_native_x(vm_t* vm) {
	var_t* cls = vm_new_class(vm, CLS_X);
	vm_reg_static(vm, cls, "open(title, x, y, w, h)", native_x_open, NULL); 
	vm_reg_static(vm, cls, "run()", native_x_run, NULL); 

	cls = vm_new_class(vm, CLS_XWIN);
	vm_reg_native(vm, cls, "setVisible(visible)", native_xwin_setVisible, NULL); 
	vm_reg_native(vm, cls, "repaint()", native_xwin_repaint, NULL); 
	vm_reg_native(vm, cls, "getSize()", native_xwin_getSize, NULL); 
}

#ifdef __cplusplus /* __cplusplus */
}
#endif
