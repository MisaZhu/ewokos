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

/*
 * extern "C" is load-bearing here, and its absence was a real bug rather than a
 * stylistic one.  Every other header in this library - time.h, setjmp.h,
 * stdlib.h, unistd.h - wraps its declarations in the same guard; this one did
 * not, so a C++ translation unit that included it got C++ linkage for these
 * functions and then failed to link:
 *
 *     $ aarch64-none-elf-g++ -std=c++14 -c sigtest.cpp   # compiles clean
 *     $ aarch64-none-elf-nm -uC sigtest.o
 *                      U raise(int)
 *                      U signal(int, void (*)(int))
 *                      U sigaction(int, sigaction const*, sigaction*)
 *     $ aarch64-none-elf-nm -uC libewoksys.a | grep sigaction
 *     sigaction.o:     T sigaction
 *
 * The object wants the mangled name, the archive defines the unmangled one, and
 * the failure surfaces at link time in whatever program happened to be first to
 * call raise() from C++ - nowhere near this file.  The compile succeeding is
 * what makes it easy to miss.
 *
 * The typedefs and struct sigaction go inside the block too.  That changes no
 * linkage - types have none - but it keeps the whole declaration set in one
 * place instead of splitting it around the guard.  Note that `struct sigaction`
 * and the function `sigaction` sharing a name is fine in C++ as well as in C;
 * it is how glibc spells it too, and the compile above confirms GCC accepts it.
 */
#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long sigset_t;

typedef void (*sighandler_t)(int signum);

/*
 * ISO C requires sig_atomic_t: the only type guaranteed to be readable and
 * writable as a single atomic access from both a signal handler and the
 * interrupted code.  It was missing here entirely, so <csignal> could not
 * re-export it and anything spelling std::sig_atomic_t failed to compile.
 *
 * int is the right choice on every architecture this tree targets - arm,
 * aarch64, riscv and x86 all load and store a naturally aligned int in one
 * instruction, which is the whole of the guarantee.  Declaring it here rather
 * than inside the C++ wrapper keeps the name available to C callers too, which
 * is where it belongs: it is a C type from the C standard, not a C++ invention.
 */
typedef int sig_atomic_t;

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

#ifdef __cplusplus
}
#endif

#endif
