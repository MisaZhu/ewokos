#include <stdlib.h>
#include <ewoksys/syscall.h>

typedef void (*atexit_func_t)(void);

#define ATEXIT_MAX_FUNCS 32

static atexit_func_t _atexit_funcs[ATEXIT_MAX_FUNCS];
static int _atexit_count = 0;
static int _atexit_running = 0;

int atexit(void (*func)(void)) {
	if(func == NULL || _atexit_count >= ATEXIT_MAX_FUNCS)
		return -1;
	_atexit_funcs[_atexit_count++] = func;
	return 0;
}

void exit(int status) {
	if(_atexit_running == 0) {
		_atexit_running = 1;
		while(_atexit_count > 0) {
			atexit_func_t func = _atexit_funcs[--_atexit_count];
			if(func != NULL)
				func();
		}
	}
	syscall1(SYS_EXIT, (ewokos_addr_t)status);
}
