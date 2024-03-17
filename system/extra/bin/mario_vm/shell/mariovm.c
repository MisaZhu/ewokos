#include "mario.h"
#include "native_builtin.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#define ERR_MAX 1023
char _err_info[ERR_MAX+1];

/**
load extra native libs.
*/
typedef void (*reg_natives_t)(vm_t* vm);

void reg_natives(vm_t* vm) {
	reg_basic_natives(vm);
}

mstr_t* load_script_content(const char* fname) {
	int fd = open(fname, O_RDONLY);
	if(fd < 0) {
		return NULL;
	}

	struct stat st;
	fstat(fd, &st);

	mstr_t* ret = mstr_new_by_size(st.st_size+1);
	int sz = read(fd, ret->cstr, st.st_size);
	close(fd);
	
	if(sz != st.st_size) {
		mstr_free(ret);
		return NULL;
	}
	ret->cstr[sz] = 0;
	ret->len = sz;
	return ret;
}

#define DEF_LIBS "/usr/local/mario"

mstr_t* include_script(vm_t* vm, const char* name) {
	const char* path = getenv("MARIO_PATH");
	if(path == NULL)
		path = DEF_LIBS;

	mstr_t* ret = load_script_content(name);
	if(ret != NULL) {
		return ret;
	}

	mstr_t* fname = mstr_new(path);
	mstr_append(fname, "/libs/");
	mstr_append(fname, _mario_lang);
	mstr_add(fname, '/');
	mstr_append(fname, name);
	
	ret = load_script_content(name);
	mstr_free(fname);
	return ret;
}

void init_args(vm_t* vm, int argc, char** argv) {
	var_t* args = var_new_array(vm);
	int i;
	for(i=0; i<argc; i++) {
		var_t* v = var_new_str(vm, argv[i]);
		var_array_add(args, v);
		var_unref(v);
	}

	var_add(vm->root, "_args", args);
}

bool load_js(vm_t* vm, const char* fname) {
	mstr_t* s = load_script_content(fname);
	if(s == NULL) {
		snprintf(_err_info, ERR_MAX, "Can not open file '%s'\n", fname);
		mario_debug(_err_info);
		return false;
	}
	
	bool ret = vm_load(vm, s->cstr);
	mstr_free(s);
	return ret;
}

void run_shell(vm_t* vm);

int main(int argc, char** argv) {
	setbuf(stdin, NULL);
	setbuf(stdout, NULL);
	if(argc < 2) {
		mario_debug("Usage: mario (-v) <js-filename>\n");
	}

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
	
	bool loaded = true;
	_load_m_func = include_script;

	mario_mem_init();
	vm_t* vm = vm_new(compile);
	vm->gc_buffer_size = 1024;
	init_args(vm, argc, argv);

	if(loaded) {
		vm_init(vm, reg_natives, NULL);

		if(fname[0] != 0) {
			mario_debug("-------- run script --------\n");
			if(load_js(vm, fname)) {
				if(verify)
					vm_dump(vm);
				else
					vm_run(vm);
			}
		}
	}
	
	vm_close(vm);
	mario_mem_close();
	return 0;
}
