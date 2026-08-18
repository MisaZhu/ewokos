#include <sched.h>
#include <errno.h>
#include <ewoksys/syscall.h>

int sched_yield(void) {
    syscall0(SYS_YIELD);
    return 0;
}

int sched_get_priority_min(int policy) {
    (void)policy;
    return 0;
}

int sched_get_priority_max(int policy) {
    (void)policy;
    return 0;
}

int sched_setscheduler(pid_t pid, int policy, const struct sched_param *param) {
    (void)pid;
    (void)param;
    if (policy != SCHED_OTHER && policy != SCHED_FIFO && policy != SCHED_RR) {
        errno = EINVAL;
        return -1;
    }
    return 0;
}

int sched_getscheduler(pid_t pid) {
    (void)pid;
    return SCHED_OTHER;
}
