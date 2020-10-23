#include <sys/signal.h>
#include <sys/syscall.h>
#include <string.h>
#include <stdlib.h>

static signal_handler_t _signals[SYS_SIG_NUM];

static void _do_signal(int sig_no) {
	if(sig_no < 0 || sig_no >= SYS_SIG_NUM)
		return;
	_signals[sig_no](sig_no);
	syscall0(SYS_SIGNAL_END);
}

void sys_sig_ignore(int sig_no) {
	(void)sig_no;
	return;
}

void sys_sig_default(int sig_no) {
	switch(sig_no) {
	case SYS_SIG_STOP:
	case SYS_SIG_KILL:
		exit(0);
		return;
	}
}

void sys_signal_init(void) {
	int i;
	for(i=0; i<SYS_SIG_NUM; i++) {
		_signals[i] = sys_sig_default;
	}
	syscall1(SYS_SIGNAL_SETUP, (int32_t)_do_signal);
}

signal_handler_t sys_signal(int sig_no, signal_handler_t handler) {
	if(sig_no < 0 || sig_no >= SYS_SIG_NUM)
		return NULL;
	signal_handler_t ret = _signals[sig_no];
	_signals[sig_no] = handler;
	return ret;
}
