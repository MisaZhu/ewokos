#include <unistd.h>
#include <errno.h>

/* EwokOS has no privilege model: effective ids equal real ids. */
uid_t geteuid(void) {
    return getuid();
}

gid_t getegid(void) {
    return getgid();
}

/* Process groups / sessions are not implemented; each process is its own group. */
pid_t getpgid(pid_t pid) {
    return (pid == 0) ? getpid() : pid;
}

pid_t getpgrp(void) {
    return getpid();
}

int setpgid(pid_t pid, pid_t pgid) {
    (void)pid;
    (void)pgid;
    return 0;
}

pid_t setsid(void) {
    return getpid();
}

pid_t getsid(pid_t pid) {
    return (pid == 0) ? getpid() : pid;
}

int nice(int inc) {
    (void)inc;
    errno = 0;
    return 0;
}
