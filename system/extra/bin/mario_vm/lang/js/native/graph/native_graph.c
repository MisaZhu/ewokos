#include "native_graph.h"
#include <ewoksys/klog.h>
#include <graph/graph.h>

#define CLS_GRAPH "Graph"

#ifdef __cplusplus /* __cplusplus */
extern "C" {
#endif

var_t* native_graph_clear(vm_t* vm, var_t* env, void* data) {
	int color = get_int(env, "color");

	graph_t* g = (graph_t*)get_raw(env, THIS);
	if(g == NULL)
		return NULL;

	graph_clear(g, color);
	return NULL;
}

var_t* native_graph_fill(vm_t* vm, var_t* env, void* data) {
	int x = get_int(env, "x");
	int y = get_int(env, "y");
	int w = get_int(env, "w");
	int h = get_int(env, "h");
	int color = get_int(env, "color");

	graph_t* g = (graph_t*)get_raw(env, THIS);
	if(g == NULL)
		return NULL;

	graph_fill(g, x, y, w, h, color);
	return NULL;
}

var_t* native_graph_box(vm_t* vm, var_t* env, void* data) {
	int x = get_int(env, "x");
	int y = get_int(env, "y");
	int w = get_int(env, "w");
	int h = get_int(env, "h");
	int color = get_int(env, "color");

	graph_t* g = (graph_t*)get_raw(env, THIS);
	if(g == NULL)
		return NULL;

	graph_box(g, x, y, w, h, color);
	return NULL;
}

var_t* native_graph_line(vm_t* vm, var_t* env, void* data) {
	int x0 = get_int(env, "x0");
	int y0 = get_int(env, "y0");
	int x1 = get_int(env, "x1");
	int y1 = get_int(env, "x1");
	int color = get_int(env, "color");

	graph_t* g = (graph_t*)get_raw(env, THIS);
	if(g == NULL)
		return NULL;

	graph_line(g, x0, y0, x1, y1, color);
	return NULL;
}

void reg_native_graph(vm_t* vm) {
	var_t* cls = vm_new_class(vm, CLS_GRAPH);
	vm_reg_native(vm, cls, "clear(color)", native_graph_clear, NULL); 
	vm_reg_native(vm, cls, "fill(x,y,w,h,color)", native_graph_fill, NULL); 
	vm_reg_native(vm, cls, "box(x,y,w,h,color)", native_graph_box, NULL); 
	vm_reg_native(vm, cls, "line(x0,y0,x1,y1,color)", native_graph_line, NULL); 
}

#ifdef __cplusplus /* __cplusplus */
}
#endif
