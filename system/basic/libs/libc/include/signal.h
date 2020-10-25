#ifndef SIGNAL_H
#define SIGNAL_H

#include <sys/signal.h>

#define SIGSTOP SYS_SIG_STOP
#define SIGTERM SYS_SIG_STOP
#define SIGKILL SYS_SIG_KILL

typedef void (*sighandler_t)(int signum);

sighandler_t signal(int signum, sighandler_t handler);
int          kill(int pid, int sig);

#endif
