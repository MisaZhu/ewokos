#include "native_graph.h"
#include <ewoksys/klog.h>
#include <graph/graph.h>
#include <upng/upng.h>
#include <font/font.h>

#define CLS_GRAPH "Graph"
#define CLS_FONT "Font"
#define CLS_PNG "PNG"

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

var_t* native_graph_round(vm_t* vm, var_t* env, void* data) {
	int x = get_int(env, "x");
	int y = get_int(env, "y");
	int w = get_int(env, "w");
	int h = get_int(env, "h");
	int r = get_int(env, "r");
	int color = get_int(env, "color");

	graph_t* g = (graph_t*)get_raw(env, THIS);
	if(g == NULL)
		return NULL;

	graph_round(g, x, y, w, h, r, color);
	return NULL;
}

var_t* native_graph_fillRound(vm_t* vm, var_t* env, void* data) {
	int x = get_int(env, "x");
	int y = get_int(env, "y");
	int w = get_int(env, "w");
	int h = get_int(env, "h");
	int r = get_int(env, "r");
	int color = get_int(env, "color");

	graph_t* g = (graph_t*)get_raw(env, THIS);
	if(g == NULL)
		return NULL;

	graph_fill_round(g, x, y, w, h, r, color);
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
	int size = get_int(env, "size");
	int color = get_int(env, "color");

	graph_t* g = (graph_t*)get_raw(env, THIS);
	if(g == NULL || font == NULL)
		return NULL;

	graph_draw_text_font(g, x, y, text, font, size, color);
	return NULL;
}

var_t* native_graph_blt(vm_t* vm, var_t* env, void* data) {
	graph_t* src = (graph_t*)get_raw(env, "src");
	int sx = get_int(env, "sx");
	int sy = get_int(env, "sy");
	int sw = get_int(env, "sw");
	int sh = get_int(env, "sh");

	graph_t* dst = (graph_t*)get_raw(env, THIS);
	if(src == NULL || dst == NULL)
		return NULL;
	
	int dx = get_int(env, "dx");
	int dy = get_int(env, "dy");
	int dw = get_int(env, "dw");
	int dh = get_int(env, "dh");

	graph_blt(src, sx, sy, sw, sh, dst, dx, dy, dw, dh);
	return NULL;
}

var_t* native_graph_bltAlpha(vm_t* vm, var_t* env, void* data) {
	graph_t* src = (graph_t*)get_raw(env, "src");
	int sx = get_int(env, "sx");
	int sy = get_int(env, "sy");
	int sw = get_int(env, "sw");
	int sh = get_int(env, "sh");


	graph_t* dst = (graph_t*)get_raw(env, THIS);
	if(src == NULL || dst == NULL)
		return NULL;
	
	int dx = get_int(env, "dx");
	int dy = get_int(env, "dy");
	int dw = get_int(env, "dw");
	int dh = get_int(env, "dh");

	int alpha = get_int(env, "alpha");

	graph_blt_alpha(src, sx, sy, sw, sh, dst, dx, dy, dw, dh, alpha);
	return NULL;
}

//===================Font natives=================
static void free_font(void* p) {
	font_t* font = (font_t*)p;
	font_free(font);
}

var_t* native_font_constructor(vm_t* vm, var_t* env, void* data) {
	const char* name = get_str(env, "name");
	font_t* font = font_new(name, true);

	var_t* thisV = var_new_obj(vm, font, (free_func_t)free_font);
	var_instance_from(thisV, get_obj(env, THIS));
	return thisV;
}

//===================Png natives=================
static void free_g(void* p) {
	graph_t* img = (graph_t*)p;
	graph_free(img);
}

var_t* native_png_load(vm_t* vm, var_t* env, void* data) {
	const char* name = get_str(env, "name");
	graph_t* img = png_image_new(name);
	if(img == NULL)
		return NULL;

	var_t* v = new_obj(vm, CLS_GRAPH, 0);
	v->value = img;
	v->free_func = free_g;
	var_add(v, "width", var_new_int(vm, img->w));
	var_add(v, "height", var_new_int(vm, img->h));

	return v;
}

void reg_native_graph(vm_t* vm) {
	var_t* cls = vm_new_class(vm, CLS_GRAPH);
	vm_reg_native(vm, cls, "clear(color)", native_graph_clear, NULL); 
	vm_reg_native(vm, cls, "fill(x,y,w,h,color)", native_graph_fill, NULL); 
	vm_reg_native(vm, cls, "round(x,y,w,h,r,color)", native_graph_round, NULL); 
	vm_reg_native(vm, cls, "fillRound(x,y,w,h,r,color)", native_graph_fillRound, NULL); 
	vm_reg_native(vm, cls, "box(x,y,w,h,color)", native_graph_box, NULL); 
	vm_reg_native(vm, cls, "line(x0,y0,x1,y1,color)", native_graph_line, NULL); 
	vm_reg_native(vm, cls, "drawText(x, y, text, font, size, color)", native_graph_drawText, NULL); 
	vm_reg_native(vm, cls, "blt(src, sx, sy, sw, sh, dx, dy, dw, dh)", native_graph_blt, NULL); 
	vm_reg_native(vm, cls, "bltAlpha(src, sx, sy, sw, sh, dx, dy, dw, dh, alpha)", native_graph_bltAlpha, NULL); 

	cls = vm_new_class(vm, CLS_FONT);
	vm_reg_native(vm, cls, "constructor(name)", native_font_constructor, NULL); 

	cls = vm_new_class(vm, CLS_PNG);
	vm_reg_static(vm, cls, "load(name)", native_png_load, NULL); 
}

#ifdef __cplusplus /* __cplusplus */
}
#endif
