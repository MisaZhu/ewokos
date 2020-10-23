#ifndef SIGNAL_H
#define SIGNAL_H

#include <sys/signal.h>

#define SIGSTOP SYS_SIG_STOP
#define SIGKILL SYS_SIG_KILL

signal_handler_t signal(int signum, signal_handler_t handler);

#endif
