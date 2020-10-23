#include <signal.h>

signal_handler_t signal(int signum, signal_handler_t handler) {
	return sys_signal(signum, handler);
}
