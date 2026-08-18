#include <sys/resource.h>
#include <string.h>
#include <errno.h>
#include <ewoksys/proc.h>

int getrusage(int who, struct rusage *usage) {
    if (usage == NULL || (who != RUSAGE_SELF && who != RUSAGE_CHILDREN)) {
        errno = EINVAL;
        return -1;
    }
    /* No per-process accounting is available from the kernel. */
    memset(usage, 0, sizeof(*usage));
    return 0;
}

int getrlimit(int resource, struct rlimit *rlp) {
    if (rlp == NULL || resource < 0 || resource >= RLIMIT_NLIMITS) {
        errno = EINVAL;
        return -1;
    }
    if (resource == RLIMIT_NOFILE) {
        rlp->rlim_cur = MAX_OPEN_FILE_PER_PROC;
        rlp->rlim_max = MAX_OPEN_FILE_PER_PROC;
    } else {
        rlp->rlim_cur = RLIM_INFINITY;
        rlp->rlim_max = RLIM_INFINITY;
    }
    return 0;
}

int setrlimit(int resource, const struct rlimit *rlp) {
    if (rlp == NULL || resource < 0 || resource >= RLIMIT_NLIMITS) {
        errno = EINVAL;
        return -1;
    }
    return 0; /* accepted, not enforced */
}

int getpriority(int which, id_t who) {
    (void)who;
    if (which != PRIO_PROCESS && which != PRIO_PGRP && which != PRIO_USER) {
        errno = EINVAL;
        return -1;
    }
    errno = 0;
    return 0;
}

int setpriority(int which, id_t who, int value) {
    (void)who;
    (void)value;
    if (which != PRIO_PROCESS && which != PRIO_PGRP && which != PRIO_USER) {
        errno = EINVAL;
        return -1;
    }
    return 0;
}
