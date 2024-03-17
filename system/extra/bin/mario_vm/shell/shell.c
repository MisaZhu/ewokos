#include "mario.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

void print(const char* msg) {
	write(0, msg, strlen(msg));
}

void line_number(int l) {
	print(mstr_from_int(l, 10));
	print("  ");
}

void show_code(m_array_t* lines) {
	int i;
	for(i=0; i<lines->size; ++i) {
		mstr_t* l = (mstr_t*)array_get(lines, i);
		line_number(i);
		print(l->cstr);
	}
}

void run(vm_t* vm, m_array_t* lines) {
	if(lines->size == 0) {
		print("(empty)\n"); //do nothing for empty source.
		return;
	}

	int i;
	mstr_t* src = mstr_new("");
	for(i=0; i<lines->size; ++i) {
		mstr_t* l = (mstr_t*)array_get(lines, i);
		mstr_append(src, l->cstr);
	}
	vm_t* vmr = vm_from(vm);
	if(vm_load(vmr, src->cstr)) {
		vm_run(vmr);
	}
	vm_close(vmr);
	mstr_free(src);
}

bool read_line(mstr_t* line) {
	while(true) { //read line.
		char c[1];
		if(read(0, c, 1) <= 0) {
			return false;
		}
		mstr_add(line, c[0]);
		write(1, c, 1);
		if(c[0] == '\n')
			break;
	}
	return true;
}

void run_shell(vm_t* vm) {
	const char* usage = "Usage: \n"
											"  help    : show this help info.\n"
											"  <ENTER> : switch edit/command mode.\n"
											"  exit    : quit mario shell.\n"
											"  clear   : clear source code.\n"
											"  run     : run source code.\n\n";
	const char* hint = "(source input, empty line enter switch edit/command mode)\n";

	m_array_t* lines = array_new();
	mstr_t* line = mstr_new("");

	print(usage);
	print(hint);
	bool cmd_mode = false;
	int ln = 0;
	while(!vm->terminated) {
		mstr_reset(line);
		if(cmd_mode)
			print("cmd:> ");
		else
			line_number(ln);

		if(!read_line(line)) {
			vm->terminated = true;
			break;
		}

		if(line->len == 1 && line->cstr[0] == '\n') { //enter fo switch edit/command mode
			cmd_mode = !cmd_mode;
			if(!cmd_mode)  {
				print(hint);
				show_code(lines);
			}
			continue;
		}

		if(line->len > 0 && cmd_mode) {
			if(strncmp(line->cstr, "exit\n", 5) == 0) {
				vm->terminated = true;
				break;
			}
			else if(strncmp(line->cstr, "help\n", 5) == 0) {
				print(usage);
			}
			else if(strncmp(line->cstr, "clear\n", 6) == 0) {
				array_clean(lines, (free_func_t)mstr_free);
				ln = 0;
			}
			else if(strncmp(line->cstr, "run\n", 4) == 0) {
				run(vm, lines);
			}
			else {
				print("Wrong command, try 'help'\n");
			}
		}
		else {
			array_add(lines, mstr_new(line->cstr));
			ln++;
		}
	}
	array_free(lines, (free_func_t)mstr_free);
	mstr_free(line);
}

