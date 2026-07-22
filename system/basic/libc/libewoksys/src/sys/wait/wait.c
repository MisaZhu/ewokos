#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <ewoksys/proc.h>
#include <ewoksys/wait.h>
#include <procinfo.h>
#include <sysinfo.h>

static int wait_reap_child(pid_t pid, int *status) {
	if (ewok_waitpid(pid) != 0) {
		return -1;
	}
	if (status != NULL) {
		*status = 0;
	}
	return pid;
}

static int wait_match_child(pid_t wanted, procinfo_t *info) {
	int self = getpid();

	if (info->father_pid != self) {
		return 0;
	}
	if (info->type == TASK_TYPE_THREAD) {
		return 0;
	}
	if (wanted > 0 && info->pid != wanted) {
		return 0;
	}
	return 1;
}

static pid_t wait_scan_children(pid_t wanted, int *status, int options, int *has_child) {
	procinfo_t info;

	*has_child = 0;
	for (int pid = 0; pid < MAX_PROC_NUM; ++pid) {
		if (proc_info(pid, &info) != 0) {
			continue;
		}
		if (!wait_match_child(wanted, &info)) {
			continue;
		}

		*has_child = 1;
		if (info.state == ZOMBIE) {
			return wait_reap_child(info.pid, status);
		}
	}

	if ((options & WNOHANG) != 0) {
		return 0;
	}
	return -2;
}

pid_t waitpid(pid_t pid, int *status, int options) {
	int has_child = 0;
	pid_t res;

	if ((options & ~WNOHANG) != 0) {
		errno = ENOSYS;
		return -1;
	}
	if (pid < -1 || pid == 0) {
		errno = ENOSYS;
		return -1;
	}

	if (pid > 0) {
		procinfo_t info;

		if (proc_info(pid, &info) != 0 || !wait_match_child(pid, &info)) {
			errno = ECHILD;
			return -1;
		}
		if (info.state == ZOMBIE) {
			return wait_reap_child(pid, status);
		}
		if ((options & WNOHANG) != 0) {
			return 0;
		}
		if (ewok_waitpid(pid) != 0) {
			return -1;
		}
		if (status != NULL) {
			*status = 0;
		}
		return pid;
	}

	while (1) {
		res = wait_scan_children(pid, status, options, &has_child);
		if (res >= 0) {
			return res;
		}
		if (!has_child) {
			errno = ECHILD;
			return -1;
		}
		usleep(1000);
	}
}

pid_t wait(int *status) {
	return waitpid(-1, status, 0);
}
