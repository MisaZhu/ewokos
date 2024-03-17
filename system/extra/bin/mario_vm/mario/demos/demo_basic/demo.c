#include "mario.h"

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <dirent.h>
#include <stdio.h>

#define ERR_MAX 1023
char _err_info[ERR_MAX+1];

bool load_script(vm_t* vm, const char* fname) {
	int fd = open(fname, O_RDONLY);
	if(fd < 0) {
		snprintf(_err_info, ERR_MAX, "Can not open file '%s'\n", fname);
		mario_debug(_err_info);
		return false;
	}

	struct stat st;
	fstat(fd, &st);

	char* s = (char*)_malloc(st.st_size+1);
	read(fd, s, st.st_size);
	close(fd);
	s[st.st_size] = 0;

	bool ret = vm_load(vm, s);
	_free(s);
	return ret;
}

var_t* native_print(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	var_t* v = var_find_var(env, "s");
	str_t* s = str_new("");
	var_to_str(v, s);
	str_add(s, '\n');
	_out_func(s->cstr);
	str_free(s);
	return NULL;
}

int main(int argc, char** argv) {
	bool verify = false;
	const char* fname = "";

	if(argc < 2) {
		mario_debug("Usage: demo [source_file]!\n");
		return 1;
	}

	if(strcmp(argv[1], "-v") == 0) {
		if(argc != 3)
			return 1;
		verify = true;
		fname = argv[2];
	}
	else {
		fname = argv[1];
	}

	mario_mem_init();
	
	vm_t* vm = vm_new(compile);
	vm_init(vm, NULL, NULL);
	vm_reg_static(vm, NULL, "print(s)", native_print, NULL);

	if(fname[0] != 0) {
		if(load_script(vm, fname)) {
			if(verify)
				vm_dump(vm);
			else
				vm_run(vm);
		}
	}

	vm_close(vm);
	mario_mem_close();
	return 0;
}
