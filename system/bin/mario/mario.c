#include "mario/mario_vm.h"
#include <sys/vfs.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

void init_args(vm_t* vm, int argc, char** argv) {
	var_t* args = var_new_array(vm);
	int i;
	for(i=0; i<argc; i++) {
		var_array_add(args, var_new_str(vm, argv[i]));
	}

	var_add(vm->root, "_args", args);
}

bool load_js(vm_t* vm, const char* fname, bool verify) {
	char* s = vfs_readfile(fname, NULL);
	if(s == NULL) {
		//snprintf(_err_info, ERR_MAX, "Can not open file '%s'\n", fname);
		//_err(_err_info);
		return false;
	}
	
	bool ret;
	if(verify)
		ret = vm_load(vm, s);
	else
		ret = vm_load_run(vm, s);
	free(s);
	return ret;
}

void run_shell(vm_t* vm);

int main(int argc, char** argv) {
	/*if(argc < 2) {
		_err("Usage: mario (-v) <js-filename>\n");
	}
	*/

	bool verify = false;
	const char* fname = "";

	if(argc > 1) {
		if(strcmp(argv[1], "-v") == 0) {
			if(argc != 3)
				return 1;
			verify = true;
			fname = argv[2];
		}
		else {
			fname = argv[1];
		}
	}

	vm_t* vm = vm_new(compile);
	vm->gc_buffer_size = 1024;

	init_args(vm, argc, argv);

	vm_init(vm, NULL, NULL);

	if(fname[0] != 0) {
		printf("-------- run script --------\n");
		if(load_js(vm, fname, verify)) {
			if(verify)
				vm_dump(vm);
		}
	}
	else {
		run_shell(vm);
	}
	
	vm_close(vm);
	return 0;
}
