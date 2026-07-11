#include <signal.h>
#include <stddef.h>
#include <sys/errno.h>

static sigset_t current_sigmask = 0;
static sigset_t signal_masks[SYS_SIG_NUM];
static int signal_flags[SYS_SIG_NUM];

extern sighandler_t __sig_get_handler(int signum);

static int valid_signal(int signum) {
	return signum >= 0 && signum < SYS_SIG_NUM;
}

int sigemptyset(sigset_t *set) {
	if (set == NULL) {
		errno = EINVAL;
		return -1;
	}
	*set = 0;
	return 0;
}

int sigfillset(sigset_t *set) {
	if (set == NULL) {
		errno = EINVAL;
		return -1;
	}
	*set = ~0UL;
	return 0;
}

int sigaddset(sigset_t *set, int signum) {
	if (set == NULL || !valid_signal(signum)) {
		errno = EINVAL;
		return -1;
	}
	*set |= (1UL << signum);
	return 0;
}

int sigdelset(sigset_t *set, int signum) {
	if (set == NULL || !valid_signal(signum)) {
		errno = EINVAL;
		return -1;
	}
	*set &= ~(1UL << signum);
	return 0;
}

int sigismember(const sigset_t *set, int signum) {
	if (set == NULL || !valid_signal(signum)) {
		errno = EINVAL;
		return -1;
	}
	return ((*set & (1UL << signum)) != 0) ? 1 : 0;
}

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact) {
	if (!valid_signal(signum)) {
		errno = EINVAL;
		return -1;
	}

	if (oldact != NULL) {
		oldact->sa_handler = __sig_get_handler(signum);
		if (oldact->sa_handler == SIG_ERR)
			oldact->sa_handler = SIG_DFL;
		oldact->sa_mask = signal_masks[signum];
		oldact->sa_flags = signal_flags[signum];
	}

	if (act != NULL) {
		if (signal(signum, act->sa_handler) == SIG_ERR) {
			return -1;
		}
		signal_masks[signum] = act->sa_mask;
		signal_flags[signum] = act->sa_flags;
	}

	return 0;
}

int sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {
	if (oldset != NULL) {
		*oldset = current_sigmask;
	}
	if (set == NULL) {
		return 0;
	}

	switch (how) {
	case SIG_BLOCK:
		current_sigmask |= *set;
		return 0;
	case SIG_UNBLOCK:
		current_sigmask &= ~(*set);
		return 0;
	case SIG_SETMASK:
		current_sigmask = *set;
		return 0;
	default:
		errno = EINVAL;
		return -1;
	}
}
