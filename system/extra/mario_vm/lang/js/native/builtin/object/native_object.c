#ifdef __cplusplus
extern "C" {
#endif

#include "native_object.h"

/** Object */

var_t* native_Object_create(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	var_t* proto = get_obj(env, "proto");
	var_t* ret = var_new_obj(vm, NULL, NULL);
	var_from_prototype(ret, proto);
	return ret;
}

var_t* native_Object_getPrototypeOf(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	var_t* obj = get_obj(env, "obj");
	return var_get_prototype(obj);
}

#define CLS_OBJECT "Object"

void reg_native_object(vm_t* vm) {
	var_t* cls = vm_new_class(vm, CLS_OBJECT);
	vm_reg_static(vm, cls, "create(proto)", native_Object_create, NULL); 
	vm_reg_static(vm, cls, "getPrototypeOf(obj)", native_Object_getPrototypeOf, NULL); 
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
