#include "js.h"
#include "mbc.h"
#include "platform.h"
#include "mem.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

/**
load extra native libs.
*/

void reg_natives(vm_t* vm);

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

enum {
	MODE_RUN = 0,
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

bool js_compile(bytecode_t *bc, const char* input);

int main(int argc, char** argv) {
	setbuf(stdin, NULL);
	setbuf(stdout, NULL);

	platform_init();

	if(doargs(argc, argv) != 0) {
		printf("Usage: mario (-c/d) <filename>\n");
		return -1;
	}

	bool loaded = true;

	mem_init();
	vm_t* vm = vm_new(js_compile);
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
				if(_mode == MODE_CMPL) 
					vm_gen_mbc(vm, _fname_out);
				else
					vm_run(vm);
			}
		}
	}
	
	vm_close(vm);
	mem_quit();
	return 0;
}
