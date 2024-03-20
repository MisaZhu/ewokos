#ifdef __cplusplus
extern "C" {
#endif

#include "native_console.h"

static mstr_t* args_to_str(var_t* args) {
	mstr_t* ret = mstr_new("");
	mstr_t* str = mstr_new("");
	uint32_t sz = var_array_size(args);
	uint32_t i;
	for(i=0; i<sz; ++i) {
		node_t* n = var_array_get(args, i);
		if(n != NULL) {
			var_to_str(n->var, str);
			if(i > 0)
				mstr_add(ret, ' ');
			mstr_append(ret, str->cstr);
		}
	}
	mstr_free(str);
	return ret;
}

var_t* native_print(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	var_t* v = get_func_args(env); 
	mstr_t* ret = args_to_str(v);
	_out_func(ret->cstr);
	mstr_free(ret);
	return NULL;
}

var_t* native_println(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	var_t* v = get_func_args(env); 
	mstr_t* ret = args_to_str(v);
	mstr_add(ret, '\n');
	_out_func(ret->cstr);
	mstr_free(ret);
	return NULL;
}

#define CLS_CONSOLE "console"

void reg_native_console(vm_t* vm) {
	var_t* cls = vm_new_class(vm, CLS_CONSOLE);
	vm_reg_static(vm, cls, "write(v)", native_print, NULL); 
	vm_reg_static(vm, cls, "log(v)", native_println, NULL); 
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
