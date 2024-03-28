#ifdef __cplusplus
extern "C" {
#endif

#include "native_number.h"

/** Number */

var_t* native_Number_constructor(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	var_t* this_v = NULL;
	var_t* v = get_obj(env, "value");
	if(v != NULL) {
		if(v->type == V_INT) {
			this_v = var_new_int(vm, var_get_int(v));
		}
		else if(v->type == V_FLOAT) {
			this_v = var_new_float(vm, var_get_float(v));
		}
	}

	if(this_v == NULL)
		this_v = var_new_int(vm, 0);
	var_instance_from(this_v, get_obj(env, THIS));
	return this_v;
}

var_t* native_Number_toString(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	const char *s;

	var_t* v = get_obj(env, THIS);
	if(v->type == V_INT) {
		int radix = get_int(env, "radix");
		if(radix < 2 || radix > 36)
			radix = 10;
		s = str_from_int(var_get_int(v), radix);
	}
	else {
		s = str_from_float(var_get_float(v));
	}
	var_t* ret = var_new_str(vm, s);
	return ret;
}

#define CLS_NUMBER "Number"

void reg_native_number(vm_t* vm) {
	var_t* cls = vm_new_class(vm, CLS_NUMBER);
	vm_reg_native(vm, cls, "toString(radix)", native_Number_toString, NULL); 
	vm_reg_native(vm, cls, "constructor(value)", native_Number_constructor, NULL); 
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
