#include <signal.h>
#include <stddef.h>

static void do_sys_signal(int signum, void* p) {
	sighandler_t func = (sighandler_t)p;
	func(signum);
}

sighandler_t signal(int signum, sighandler_t handler) {
	if(sys_signal(signum, do_sys_signal, handler ) == NULL)
		return NULL;
	return handler;
}
