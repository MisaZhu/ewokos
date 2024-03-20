#ifdef __cplusplus
extern "C" {
#endif

#include "native_string.h"

/** String */
var_t* native_StringConstructor(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	const char* s = get_str(env, "str");
	var_t* thisV = var_new_str(vm, s);

	var_instance_from(thisV, get_obj(env, THIS));
	return thisV;
}

var_t* native_StringLength(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	const char* s = get_str(env, THIS);
	return var_new_int(vm, (int)strlen(s));
}

var_t* native_StringToString(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	const char* s = get_str(env, THIS);
	return var_new_str(vm, s);
}

var_t* native_StringSubstr(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	const char* s = get_str(env, THIS);
	int start = get_int(env, "start");
	if(start < 0)
		start = 0;

	int length = get_int(env, "length");
	int sl = (int)strlen(s) - start;
	if(sl <= 0)
		return var_new_str(vm, "");
	if(length > sl) 
		length = sl;
	var_t* ret = var_new_str2(vm, s+start, length);
	var_instance_from(ret, get_obj(env, THIS));
	return ret;
}

var_t* native_UTF8Constructor(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	const char* s = get_str(env, "str");
	utf8_t* u = utf8_new(s);

	var_t* thisV = var_new_obj(vm, u, (free_func_t)utf8_free);
	var_instance_from(thisV, get_obj(env, THIS));
	return thisV;
}

var_t* native_UTF8Length(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	utf8_t* u = (utf8_t*)get_raw(env, THIS);
	return var_new_int(vm, utf8_len(u));
}

var_t* native_UTF8ToString(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	utf8_t* u = (utf8_t*)get_raw(env, THIS);
	mstr_t *s = mstr_new("");
	utf8_to_str(u, s);
	var_t* v = var_new_str(vm, s->cstr);
	mstr_free(s);
	return v;
}

var_t* native_UTF8At(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	utf8_t* u = (utf8_t*)get_raw(env, THIS);
	int32_t at = get_int(env, "index");

	mstr_t *s = utf8_at(u, at);
	var_t* v = var_new_str(vm, s->cstr);
	return v;
}

var_t* native_UTF8Set(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	utf8_t* u = (utf8_t*)get_raw(env, THIS);
	int32_t at = get_int(env, "index");
	const char* s = get_str(env, "s");

	utf8_set(u, at, s);
	return NULL;
}

var_t* native_UTF8Substr(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	utf8_t* u = (utf8_t*)get_raw(env, THIS);
	int start = get_int(env, "start");
	if(start < 0)
		start = 0;

	int length = get_int(env, "length");
	int sl = utf8_len(u) - start;
	utf8_t* sub = utf8_new("");
	if(sl > 0) {
		if(length > sl) 
			length = sl;
		int i;
		for(i=0; i<length; ++i) {
			mstr_t* s = utf8_at(u, i+start);
			utf8_append(sub, s->cstr);	
		}
	}
	var_t* ret = var_new_obj(vm, sub, (free_func_t)utf8_free);
	var_instance_from(ret, get_obj(env, THIS));
	return ret;
}

var_t* native_UTF8ReaderConstructor(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	const char* s = get_str(env, "str");
	utf8_reader_t* ur = (utf8_reader_t*)_malloc(sizeof(utf8_reader_t));
	utf8_reader_init(ur, s, 0);

	var_t* thisV = var_new_obj(vm, ur, NULL);
	var_instance_from(thisV, get_obj(env, THIS));
	return thisV;
}

var_t* native_UTF8ReaderRead(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	utf8_reader_t* ur = (utf8_reader_t*)get_raw(env, THIS);
	mstr_t* s = mstr_new("");
	var_t* v;
	if(utf8_read(ur, s)) 
		v = var_new_str(vm, s->cstr);
	else
		v = var_new_str(vm, "");
	mstr_free(s);
	return v;
}


#define CLS_STRING "String"
#define CLS_UTF8 "UTF8"
#define CLS_UTF8_READER "UTF8Reader"

void reg_native_string(vm_t* vm) {
	var_t* cls = vm_new_class(vm, CLS_STRING);
	vm_reg_native(vm, cls, "constructor(str)", native_StringConstructor, NULL); 
	vm_reg_native(vm, cls, "length()", native_StringLength, NULL); 
	vm_reg_native(vm, cls, "toString()", native_StringToString, NULL); 
	vm_reg_native(vm, cls, "substr(start, length)", native_StringSubstr, NULL); 

	cls = vm_new_class(vm, CLS_UTF8);
	vm_reg_native(vm, cls, "constructor(str)", native_UTF8Constructor, NULL); 
	vm_reg_native(vm, cls, "length()", native_UTF8Length, NULL); 
	vm_reg_native(vm, cls, "toString()", native_UTF8ToString, NULL); 
	vm_reg_native(vm, cls, "at(index)", native_UTF8At, NULL); 
	vm_reg_native(vm, cls, "set(index, s)", native_UTF8Set, NULL); 
	vm_reg_native(vm, cls, "substr(start, length)", native_UTF8Substr, NULL); 

	cls = vm_new_class(vm, CLS_UTF8_READER);
	vm_reg_native(vm, cls, "constructor(str)", native_UTF8ReaderConstructor, NULL); 
	vm_reg_native(vm, cls, "read()", native_UTF8ReaderRead, NULL); 
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
