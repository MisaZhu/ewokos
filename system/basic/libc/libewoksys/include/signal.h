#ifndef SIGNAL_H
#define SIGNAL_H

#include <ewoksys/signal.h>

#define SIGSTOP SYS_SIG_STOP
/* Ewok only exposes STOP/KILL internally; SIGTERM maps to the closest terminate signal. */
#define SIGTERM SYS_SIG_KILL
#define SIGKILL SYS_SIG_KILL

/*
 * The kernel only delivers SYS_SIG_STOP(0) and SYS_SIG_KILL(1). The standard
 * POSIX signal names below are provided so ported code (e.g. libcurses job
 * control / window-resize handling) compiles and can register handlers; their
 * numbers are all >= SYS_SIG_NUM, so signal()/sigaction() treat them as
 * invalid and become inert no-ops rather than aliasing the two real signals.
 */
#define SIGINT    2
#define SIGQUIT   3
#define SIGILL    4
#define SIGTRAP   5
#define SIGABRT   6
#define SIGBUS    7
#define SIGFPE    8
#define SIGUSR1   10
#define SIGSEGV   11
#define SIGUSR2   12
#define SIGPIPE   13
#define SIGALRM   14
#define SIGCHLD   17
#define SIGCONT   18
#define SIGTSTP   20
#define SIGTTIN   21
#define SIGTTOU   22
#define SIGURG    23
#define SIGWINCH  28
#define SIGIO     29
#define SIGSYS    31

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
#define SIG_HOLD ((sighandler_t)2)

sighandler_t signal(int signum, sighandler_t handler);
int          kill(int pid, int sig);
int          raise(int sig);
int          sigemptyset(sigset_t *set);
int          sigfillset(sigset_t *set);
int          sigaddset(sigset_t *set, int signum);
int          sigdelset(sigset_t *set, int signum);
int          sigismember(const sigset_t *set, int signum);
int          sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
int          sigprocmask(int how, const sigset_t *set, sigset_t *oldset);

#endif
