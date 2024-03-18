#include "native_graph.h"
#include <ewoksys/klog.h>
#include <graph/graph.h>
#include <font/font.h>

#define CLS_GRAPH "Graph"
#define CLS_FONT "Font"

#ifdef __cplusplus /* __cplusplus */
extern "C" {
#endif

//===================Graph natives=================

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

var_t* native_graph_drawText(vm_t* vm, var_t* env, void* data) {
	int x = get_int(env, "x");
	int y = get_int(env, "y");
	const char* text = get_str(env, "text");
	font_t* font = (font_t*)get_raw(env, "font");
	int color = get_int(env, "color");

	graph_t* g = (graph_t*)get_raw(env, THIS);
	if(g == NULL || font == NULL)
		return NULL;

	klog("%d, %d, %s, font %x, %x\n", x, y, text, font, color);
	graph_draw_text_font(g, x, y, text, font, color);
	return NULL;
}


//===================Font natives=================
static void free_font(void* p) {
	font_t* font = (font_t*)p;
	font_free(font);
}

var_t* native_font_constructor(vm_t* vm, var_t* env, void* data) {
	const char* name = get_str(env, "name");
	int size = get_int(env, "size");
	font_t* font = font_new(name, size, true);

	var_t* thisV = var_new_obj(vm, font, (free_func_t)free_font);
	var_instance_from(thisV, get_obj(env, THIS));
	return thisV;
}

void reg_native_graph(vm_t* vm) {
	var_t* cls = vm_new_class(vm, CLS_GRAPH);
	vm_reg_native(vm, cls, "clear(color)", native_graph_clear, NULL); 
	vm_reg_native(vm, cls, "fill(x,y,w,h,color)", native_graph_fill, NULL); 
	vm_reg_native(vm, cls, "box(x,y,w,h,color)", native_graph_box, NULL); 
	vm_reg_native(vm, cls, "line(x0,y0,x1,y1,color)", native_graph_line, NULL); 
	vm_reg_native(vm, cls, "drawText(x, y, text, font, color)", native_graph_drawText, NULL); 

	cls = vm_new_class(vm, CLS_FONT);
	vm_reg_native(vm, cls, "constructor(name, size)", native_font_constructor, NULL); 
}

#ifdef __cplusplus /* __cplusplus */
}
#endif
