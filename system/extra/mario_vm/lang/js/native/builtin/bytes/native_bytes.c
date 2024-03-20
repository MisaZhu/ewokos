#ifdef __cplusplus
extern "C" {
#endif

#include "native_bytes.h"

/** Bytes */

var_t* native_Bytes_constructor(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	var_t* thisV = get_obj(env, THIS);
	int sz = get_int(env, "size");
	if(sz <= 0)
		return thisV;

	if(thisV != NULL) {
		thisV->value = _malloc(sz+1);
		memset(thisV->value, 0, sz+1);
		thisV->size = sz;
	}
	return thisV;
}

var_t* native_Bytes_size(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	var_t* thisV = get_obj(env, THIS);
	int sz = thisV == NULL ? 0 : thisV->size;
	return var_new_int(vm, sz);
}

var_t* native_Bytes_toString(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	var_t* thisV = get_obj(env, THIS);
	const char* s = thisV == NULL ? "" : (const char*)thisV->value;
	return var_new_str(vm, s);
}

var_t* native_Bytes_at(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	int index = get_int(env, "index");

	var_t* thisV = get_obj(env, THIS);
	int sz = thisV == NULL ? 0 : thisV->size;

	int i = 0;
	if(sz > index)
		i = ((char*)thisV->value)[index];
		
	return var_new_int(vm, i);
}

var_t* native_Bytes_set(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	int index = get_int(env, "index");
	int i = get_int(env, "i");

	var_t* thisV = get_obj(env, THIS);
	int sz = thisV == NULL ? 0 : thisV->size;

	if(sz > index)
		((char*)thisV->value)[index] = i;
		
	return NULL;
}

var_t* native_Bytes_fromString(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	const char* s = get_str(env, "str");

	var_t* thisV = get_obj(env, THIS);
	if(thisV != NULL) {
		thisV->size = (uint32_t)strlen(s) + 1;
		void* p = thisV->value;
		thisV->value = _malloc(thisV->size);
		memcpy(thisV->value, s, thisV->size);
		if(p != NULL)
			_free(p);
	}
		
	return NULL;
}

#define CLS_BYTES "Bytes"

void reg_native_bytes(vm_t* vm) {
	var_t* cls = vm_new_class(vm, CLS_BYTES);
	vm_reg_native(vm, cls, "constructor(size)", native_Bytes_constructor, NULL); 
	vm_reg_native(vm, cls, "size()", native_Bytes_size, NULL); 
	vm_reg_native(vm, cls, "toString()", native_Bytes_toString, NULL); 
	vm_reg_native(vm, cls, "fromString(str)", native_Bytes_fromString, NULL); 
	vm_reg_native(vm, cls, "set(index, i)", native_Bytes_set, NULL); 
	vm_reg_native(vm, cls, "at(index)", native_Bytes_at, NULL); 
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
