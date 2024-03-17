#include "mario.h"

const char* js = " \
	function jsFunc(s, n) { \
		return \"Hello '\" + s + \"' (\" + n + \")!\n\"; \
	} \
	";

int main(int argc, char** argv) {
	vm_t* vm = vm_new(compile);
	vm_init(vm, NULL, NULL); //initialize the vm enviroment.

	vm_load_run(vm, js); 

	var_t* args = var_new(vm);
	var_add(args, "", var_new_str(vm, "JS world"));
	var_add(args, "", var_new_int(vm, 100));
	var_t* ret = call_m_func_by_name(vm, vm->root, "jsFunc", args);
	if(ret != NULL) {
		const char* s = var_get_str(ret);
		_out_func(s);
		var_unref(ret);
	}
	
	vm_close(vm); //release
	return 0;
}
