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

bool js_compile(bytecode_t *bc, const char* input);
void bc_dump_out(bytecode_t* bc);

int main(int argc, char** argv) {
	setbuf(stdin, NULL);
	setbuf(stdout, NULL);

	platform_init();

	if(argc < 2) {
		printf("Usage: bcasm <filename>\n");
		return -1;
	}

	const char* fname = argv[1];

	bool loaded = true;

	mem_init();
	vm_t* vm = vm_new(js_compile);
	vm->gc_buffer_size = 1024;

	if(loaded) {
		vm_init(vm, NULL, NULL);

		if(fname[0] != 0) {
			bool res = false;
			if(strstr(fname, ".js") != NULL)
				res = load_js(vm, fname);
			else if(strstr(fname, ".mbc") != NULL) {
				bc_release(&vm->bc);
				res = vm_load_mbc(vm, fname);
			}
			
			if(res) {
				bc_dump_out(&vm->bc);
			}
		}
	}
	
	vm_close(vm);
	mem_quit();
	return 0;
}
