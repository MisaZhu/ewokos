#ifdef __cplusplus
extern "C" {
#endif
#include "native_json.h"
#include <stdlib.h>

/**JSON functions */
var_t* native_json_stringify(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	node_t* n = var_find(env, "var");
	mstr_t* s = mstr_new("");
	if(n != NULL)
		var_to_json_str(n->var, s, 0);

	var_t* var = var_new_str(vm, s->cstr);
	mstr_free(s);
	return var;
}

var_t* native_json_parse(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	const char* s = get_str(env, "str");
	return json_parse(vm, s);
}

#define CLS_JSON "JSON"

void reg_native_json(vm_t* vm) {
	var_t* cls = vm_new_class(vm, CLS_JSON);
	vm_reg_native(vm, cls, "stringify(var)", native_json_stringify, NULL);
	vm_reg_native(vm, cls, "parse(str)", native_json_parse, NULL);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
