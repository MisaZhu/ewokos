#include <sys/signal.h>
#include <sys/syscall.h>
#include <string.h>
#include <stdlib.h>

static signal_handler_t _signals[SYS_SIGNAL_NUM];

static void _do_signal(int sig_no) {
	if(sig_no < 0 || sig_no >= SYS_SIGNAL_NUM)
		return;
	_signals[sig_no](sig_no);
}

void sig_exit_default(int sig_no) {
	klog("exit\n");
	exit(0);
}

void sys_singal_init(void) {
	memset(_signals, 0, sizeof(signal_handler_t) * SYS_SIGNAL_NUM);
	_signals[SYS_SIGNAL_EXIT] = sig_exit_default;
	syscall1(SYS_SIGNAL, (int32_t)_do_signal);
}

int sys_singal(int sig_no, signal_handler_t handler) {
	if(sig_no < 0 || sig_no >= SYS_SIGNAL_NUM)
		return -1;
	_signals[sig_no] = handler;
	return 0;
}
