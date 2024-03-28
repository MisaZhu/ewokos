#ifdef __cplusplus
extern "C" {
#endif

#include "native_array.h"

/** Array */

var_t* native_Array_constructor(vm_t* vm, var_t* env, void* data) {
    (void)vm; (void)data;
    var_t* this_v = get_obj(env, THIS);
    this_v->is_array = 1;
    var_t* members = var_new_obj(vm, NULL, NULL);
    var_add(this_v, "_ARRAY_", members);
    return this_v;
}
    

var_t* native_Array_toString(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	var_t* arr = get_obj(env, THIS);
	mstr_t* ret = mstr_new("");

	mstr_t* str = mstr_new("");
	uint32_t sz = var_array_size(arr);
	uint32_t i;
	for(i=0; i<sz; ++i) {
		node_t* n = var_array_get(arr, i);
		if(n != NULL) {
			var_to_str(n->var, str);
			if(i > 0)
				mstr_add(ret, ',');
			mstr_append(ret, str->cstr);
		}
	}
	mstr_free(str);

	var_t* v = var_new_str(vm, ret->cstr);
	mstr_free(ret);
	return v;
}

var_t* native_Array_join(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	var_t* arr = get_obj(env, THIS);
	const char* j = get_str(env, "c");
	mstr_t* ret = mstr_new("");

	mstr_t* str = mstr_new("");
	uint32_t sz = var_array_size(arr);
	uint32_t i;
	for(i=0; i<sz; ++i) {
		node_t* n = var_array_get(arr, i);
		if(n != NULL) {
			var_to_str(n->var, str);
			if(i > 0)
				mstr_append(ret, j);
			mstr_append(ret, str->cstr);
		}
	}
	mstr_free(str);

	var_t* v = var_new_str(vm, ret->cstr);
	mstr_free(ret);
	return v;
}

var_t* native_Array_forEach(vm_t* vm, var_t* env, void* data) {
	var_t* arr = get_obj(env, THIS);
	var_t* f = get_obj(env, "f");

	uint32_t sz = var_array_size(arr);
	uint32_t i;
	for(i=0; i<sz; ++i) {
		node_t* n = var_array_get(arr, i);
		if(n != NULL) {
			var_t* args = var_new(vm);
			var_add(args, "", n->var);
			var_add(args, "", var_new_int(vm, i));
			var_add(args, "", arr);
			var_t* res = call_m_func(vm, env, f, args);
			var_unref(args);
			if(res != NULL)	
				var_unref(res);
		}
	}
	return NULL;
}

var_t* native_Array_reverse(vm_t* vm, var_t* env, void* data) {
	var_t* arr = get_obj(env, THIS);
	var_array_reverse(arr);
	return arr;
}

var_t* native_Array_concat(vm_t* vm, var_t* env, void* data) {
	var_t* arr = get_obj(env, THIS);
	uint32_t args_num = get_func_args_num(env);
	uint32_t i;
	for(i=0; i<args_num; ++i) {
		var_t* arg = get_func_arg(env, i);
		if(!arg->is_array) { //not array type, just added to target array.
			var_array_add(arr, arg);
		}
		else { // array type , add members to target array.
			uint32_t sz = var_array_size(arg);
			uint32_t j;
			for(j=0; j<sz; ++j) {
				node_t* n = var_array_get(arg, j);
				if(n != NULL) {
					var_array_add(arr, n->var);
				}
			}
		}
	}
	return arr;
}

var_t* native_Array_push(vm_t* vm, var_t* env, void* data) {
	var_t* arr = get_obj(env, THIS);
	uint32_t args_num = get_func_args_num(env);
	uint32_t i;
	for(i=0; i<args_num; ++i) {
		var_t* arg = get_func_arg(env, i);
		var_array_add(arr, arg);
	}
	return arr;
}

var_t* native_Array_unshift(vm_t* vm, var_t* env, void* data) {
	var_t* arr = get_obj(env, THIS);
	uint32_t args_num = get_func_args_num(env);
	uint32_t i;
	for(i=args_num; i>0; --i) {
		var_t* arg = get_func_arg(env, i-1);
		var_array_add_head(arr, arg);
	}
	return arr;
}

var_t* native_Array_pop(vm_t* vm, var_t* env, void* data) {
	var_t* arr = get_obj(env, THIS);
	var_t* ret = NULL;
	uint32_t sz = var_array_size(arr);
	if(sz == 0)
		return NULL;
	
	node_t* n = var_array_remove(arr, sz-1);
	ret = var_ref(n->var);
	node_free(n);
	var_unref(ret);
	return ret;
}

var_t* native_Array_shift(vm_t* vm, var_t* env, void* data) {
	var_t* arr = get_obj(env, THIS);
	var_t* ret = NULL;
	uint32_t sz = var_array_size(arr);
	if(sz == 0)
		return NULL;
	
	node_t* n = var_array_remove(arr, 0);
	ret = var_ref(n->var);
	node_free(n);
	var_unref(ret);
	return ret;
}

var_t* native_Array_slice(vm_t* vm, var_t* env, void* data) {
	var_t* arr = get_obj(env, THIS);
	uint32_t sz = var_array_size(arr);
	int32_t start = get_int(env, "start");
	if(start < 0) 
		start = sz + start;

	int32_t end;
	var_t* end_var= get_obj(env, "end");
	if(end_var == NULL || end_var->type == V_UNDEF) 
		end = sz;
	else 
		end = var_get_int(end_var);
	if(end < 0) end = sz + end;

	uint32_t i;
	var_t* ret = var_new_array(vm);
	for(i=start; i<end; ++i) {
		node_t* n = var_array_get(arr, i);
		if(n != NULL) {
			var_array_add(ret, n->var);
		}
	}
	var_instance_from(ret, get_obj(env, THIS));
	return ret;
}

#define CLS_ARRAY "Array"

void reg_native_array(vm_t* vm) {
  var_t* cls = vm_new_class(vm, CLS_ARRAY);
	vm_reg_native(vm, cls, "constructor()", native_Array_constructor, NULL);
	vm_reg_native(vm, cls, "toString()", native_Array_toString, NULL); 
	vm_reg_native(vm, cls, "forEach(f)", native_Array_forEach, NULL); 
	vm_reg_native(vm, cls, "reverse()", native_Array_reverse, NULL); 
	vm_reg_native(vm, cls, "concat()", native_Array_concat, NULL); 
	vm_reg_native(vm, cls, "join(c)", native_Array_join, NULL); 
	vm_reg_native(vm, cls, "push()", native_Array_push, NULL); 
	vm_reg_native(vm, cls, "pop()", native_Array_pop, NULL); 
	vm_reg_native(vm, cls, "shift()", native_Array_shift, NULL); 
	vm_reg_native(vm, cls, "unshift()", native_Array_unshift, NULL); 
	vm_reg_native(vm, cls, "slice(start, end)", native_Array_slice, NULL); 
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
