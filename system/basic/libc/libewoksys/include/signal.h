#ifndef SIGNAL_H
#define SIGNAL_H

#include <ewoksys/signal.h>

#define SIGSTOP SYS_SIG_STOP
/* Ewok only exposes STOP/KILL internally; SIGTERM maps to the closest terminate signal. */
#define SIGTERM SYS_SIG_KILL
#define SIGKILL SYS_SIG_KILL

typedef unsigned long sigset_t;

typedef void (*sighandler_t)(int signum);

struct sigaction {
	sighandler_t sa_handler;
	sigset_t sa_mask;
	int sa_flags;
};

#define SIG_BLOCK   0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2

#define SA_RESTART   0x01
#define SA_INTERRUPT 0x02
#define SA_RESETHAND 0x04

#undef SIG_DFL
#undef SIG_IGN
#define SIG_DFL ((sighandler_t)0)
#define SIG_IGN ((sighandler_t)1)
#define SIG_ERR ((sighandler_t)-1)

sighandler_t signal(int signum, sighandler_t handler);
int          kill(int pid, int sig);
int          sigemptyset(sigset_t *set);
int          sigfillset(sigset_t *set);
int          sigaddset(sigset_t *set, int signum);
int          sigdelset(sigset_t *set, int signum);
int          sigismember(const sigset_t *set, int signum);
int          sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
int          sigprocmask(int how, const sigset_t *set, sigset_t *oldset);

#endif
