#include <signal.h>
#include <stddef.h>

static sighandler_t signal_handlers[SYS_SIG_NUM] = {
	SIG_DFL, SIG_DFL
};

sighandler_t __sig_get_handler(int signum) {
	if (signum < 0 || signum >= SYS_SIG_NUM) {
		return SIG_ERR;
	}
	return signal_handlers[signum];
}

static void do_sys_signal(int signum, void* p) {
	sighandler_t func = (sighandler_t)p;
	func(signum);
}

sighandler_t signal(int signum, sighandler_t handler) {
	sighandler_t old_handler;

	if (signum < 0 || signum >= SYS_SIG_NUM) {
		return SIG_ERR;
	}
	old_handler = signal_handlers[signum];

	if(handler == SIG_DFL) {
		if(sys_signal(signum, sys_sig_default, NULL) == NULL)
			return SIG_ERR;
		signal_handlers[signum] = SIG_DFL;
		return old_handler;
	}

	if(handler == SIG_IGN) {
		if(sys_signal(signum, sys_sig_ignore, NULL) == NULL)
			return SIG_ERR;
		signal_handlers[signum] = SIG_IGN;
		return old_handler;
	}

	if(sys_signal(signum, do_sys_signal, handler) == NULL)
		return SIG_ERR;
	signal_handlers[signum] = handler;
	return old_handler;
}
