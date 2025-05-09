#include <ewoksys/signal.h>
#include <ewoksys/syscall.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
	signal_handler_t func;
	void* data;
} signal_t;

static signal_t _signals[SYS_SIG_NUM];

static void _do_signal(int sig_no) {
	if(sig_no < 0 ||
			sig_no >= SYS_SIG_NUM ||
			_signals[sig_no].func == NULL)
		return;
	if(sig_no == SYS_SIG_KILL) {
		exit(0);
	}

	_signals[sig_no].func(sig_no, _signals[sig_no].data);
	syscall0(SYS_SIGNAL_END);
}

void sys_sig_ignore(int sig_no, void* p) {
	(void)sig_no;
	(void)p;
	return;
}

void sys_sig_default(int sig_no, void* p) {
	(void)p;
	switch(sig_no) {
	case SYS_SIG_STOP:
		exit(0);
		return;
	}
}

void sys_signal_init(void) {
	int i;
	for(i=0; i<SYS_SIG_NUM; i++) {
		_signals[i].func = sys_sig_default;
		_signals[i].data = NULL;
	}
	syscall1(SYS_SIGNAL_SETUP, (ewokos_addr_t)_do_signal);
}

signal_handler_t sys_signal(int sig_no, signal_handler_t handler, void* p) {
	if(sig_no < 0 || sig_no >= SYS_SIG_NUM)
		return NULL;
	_signals[sig_no].func = handler;
	_signals[sig_no].data = p;
	return handler;
}
