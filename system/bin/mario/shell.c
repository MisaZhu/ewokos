#include "mario/mario_vm.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

void line_number(int l) {
	printf("%d  ", l);
}

void show_code(m_array_t* lines) {
	int i;
	for(i=0; i<lines->size; ++i) {
		str_t* l = (str_t*)array_get(lines, i);
		line_number(i);
		printf("%s", l->cstr);
	}
}

void run(vm_t* vm, m_array_t* lines) {
	if(lines->size == 0) {
		printf("(empty)\n"); //do nothing for empty source.
		return;
	}

	int i;
	str_t* src = str_new("");
	for(i=0; i<lines->size; ++i) {
		str_t* l = (str_t*)array_get(lines, i);
		str_add(src, l->cstr);
	}
	vm_t* vmr = vm_from(vm);
	if(vm_load(vmr, src->cstr)) {
		vm_run(vmr);
	}
	vm_close(vmr);
	str_free(src);
}

#define KEY_BACKSPACE 127
#define KEY_LEFT 0x8

static int32_t read_line(str_t* buf) {
  str_reset(buf);
  while(1) {
    int c = getch();
    if(c == 0)
      return -1;

    if (c == KEY_BACKSPACE) {
      if (buf->len > 0) {
        //delete last char
        putch(KEY_LEFT);
        putch(' ');
        putch(KEY_LEFT);
        buf->len--;
      }
    }
    else if (c == 8) {
      if (buf->len > 0) {
        //delete last char
        putch(c);
        buf->len--;
      }
    }
    else {
      putch(c);
      if(c == '\r' || c == '\n')
        break;
      str_addc(buf, c);
    }
  }
  str_addc(buf, 0);
  return 0;
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
	str_t* line = str_new("");

	printf("%s", usage);
	printf("%s", hint);
	bool cmd_mode = false;
	int ln = 0;
	while(!vm->terminated) {
		str_reset(line);
		if(cmd_mode)
			printf("cmd:> ");
		else
			line_number(ln);

		if(read_line(line) != 0) {
			vm->terminated = true;
			break;
		}

		if(line->len == 0) { //enter fo switch edit/command mode
			cmd_mode = !cmd_mode;
			if(!cmd_mode)  {
				printf(hint);
				show_code(lines);
			}
			continue;
		}

		if(line->len > 0 && cmd_mode) {
			if(strncmp(line->cstr, "exit", 4) == 0) {
				vm->terminated = true;
				break;
			}
			else if(strncmp(line->cstr, "help", 4) == 0) {
				printf(usage);
			}
			else if(strncmp(line->cstr, "clear", 5) == 0) {
				array_clean(lines, (free_func_t)str_free);
				ln = 0;
			}
			else if(strncmp(line->cstr, "run", 3) == 0) {
				run(vm, lines);
			}
			else {
				printf("Wrong command, try 'help'\n");
			}
		}
		else {
			str_addc(line, '\n');
			array_add(lines, str_new(line->cstr));
			ln++;
		}
	}
	array_free(lines, (free_func_t)str_free);
	str_free(line);
}

