#include "mario.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

/**
load extra native libs.
*/

void reg_natives(vm_t* vm);

mstr_t* load_script_content(const char* fname) {
	int fd = open(fname, O_RDONLY);
	if(fd < 0) {
		return NULL;
	}

	struct stat st;
	fstat(fd, &st);

	mstr_t* ret = mstr_new_by_size(st.st_size+1);

	char* p = ret->cstr;
	uint32_t sz = st.st_size;
	while(sz > 0) {
		int rd = read(fd, p, sz);
		if(rd <= 0 || sz < rd)
			break;
		p += rd;
		sz -= rd; 
	}
	close(fd);
	
	if(sz > 0) {
		mstr_free(ret);
		return NULL;
	}
	ret->cstr[st.st_size] = 0;
	ret->len = st.st_size;
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
		printf("Can not open file '%s'\n", fname);
		return false;
	}
	
	bool ret = vm_load(vm, s->cstr);
	mstr_free(s);
	return ret;
}

enum {
	MODE_RUN = 0,
	MODE_ASM,
	MODE_CMPL
};

static uint8_t _mode = MODE_RUN; //0 for run, 1 for verify, 2 for generate mbc file
static const char* _fname = "";
static const char* _fname_out = "";

static int doargs(int argc, char* argv[]) {
	int c = 0;
	while (c != -1) {
		c = getopt (argc, argv, "cvd");
		if(c == -1)
			break;

		switch (c) {
		case 'c':
			_mode = MODE_CMPL;
			break;
		case 'v':
			_mode = MODE_ASM;
			break;
		case 'd':
			_m_debug = true;
			break;
		case '?':
			return -1;
		default:
			c = -1;
			break;
		}
	}

	if(optind < 0 || optind == argc)
		return -1;

	_fname = argv[optind];
	optind++;
	if(optind < argc)
		_fname_out = argv[optind];
	return 0;
}

void vm_gen_mbc(vm_t* vm, const char* fname_out);
bool vm_load_mbc(vm_t* vm, const char* fname);

int main(int argc, char** argv) {
	setbuf(stdin, NULL);
	setbuf(stdout, NULL);

	if(doargs(argc, argv) != 0) {
		printf("Usage: mario (-v/c/d) <filename>\n");
		return -1;
	}

	bool loaded = true;
	_load_m_func = include_script;

	mario_mem_init();
	vm_t* vm = vm_new(compile);
	vm->gc_buffer_size = 1024;
	init_args(vm, argc, argv);

	if(loaded) {
		vm_init(vm, reg_natives, NULL);

		if(_fname[0] != 0) {
			bool res = false;
			if(strstr(_fname, ".js") != NULL)
				res = load_js(vm, _fname);
			else if(strstr(_fname, ".mbc") != NULL && _mode != 2) {
				bc_release(&vm->bc);
				res = vm_load_mbc(vm, _fname);
			}
			
			if(res) {
				if(_mode == 1)
					vm_dump(vm);
				else if(_mode == 2)
					vm_gen_mbc(vm, _fname_out);
				else
					vm_run(vm);
			}
		}
	}
	
	vm_close(vm);
	mario_mem_close();
	return 0;
}
