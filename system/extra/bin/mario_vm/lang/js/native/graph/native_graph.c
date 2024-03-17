#include "native_graph.h"
#include <ewoksys/klog.h>
#include <graph/graph.h>

#define CLS_GRAPH "Graph"

#ifdef __cplusplus /* __cplusplus */
extern "C" {
#endif

static void free_none(void* p) {
	return;
}

var_t* native_graph_clear(vm_t* vm, var_t* env, void* data) {
	int color = get_int(env, "bgColor");

	graph_t* g = (graph_t*)get_raw(env, THIS);
	klog("g 0x%x\n", g);
	if(g != NULL)
		graph_clear(g, color);
	return NULL;
}

void reg_native_graph(vm_t* vm) {
	var_t* cls = vm_new_class(vm, CLS_GRAPH);
	vm_reg_native(vm, cls, "clear(bgColor)", native_graph_clear, NULL); 
}

#ifdef __cplusplus /* __cplusplus */
}
#endif
