#ifndef EWOKOS_SYS_RESOURCE_H
#define EWOKOS_SYS_RESOURCE_H

#include <sys/types.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RUSAGE_SELF     0
#define RUSAGE_CHILDREN (-1)

#define RLIM_INFINITY  (~0UL)
#define RLIM_SAVED_MAX (~0UL)
#define RLIM_SAVED_CUR (~0UL)

#define RLIMIT_CORE   0
#define RLIMIT_CPU    1
#define RLIMIT_DATA   2
#define RLIMIT_FSIZE  3
#define RLIMIT_NOFILE 4
#define RLIMIT_STACK  5
#define RLIMIT_AS     6
#define RLIMIT_NPROC  7
#define RLIMIT_RSS    8
#define RLIMIT_MEMLOCK 9
#define RLIMIT_NLIMITS 10

#define PRIO_PROCESS 0
#define PRIO_PGRP    1
#define PRIO_USER    2

struct rusage {
	struct timeval ru_utime;
	struct timeval ru_stime;
	long ru_maxrss;
	long ru_ixrss;
	long ru_idrss;
	long ru_isrss;
	long ru_minflt;
	long ru_majflt;
	long ru_nswap;
	long ru_inblock;
	long ru_oublock;
	long ru_msgsnd;
	long ru_msgrcv;
	long ru_nsignals;
	long ru_nvcsw;
	long ru_nivcsw;
};

typedef unsigned long rlim_t;

struct rlimit {
	rlim_t rlim_cur;
	rlim_t rlim_max;
};

int getrusage(int who, struct rusage *usage);
int getrlimit(int resource, struct rlimit *rlp);
int setrlimit(int resource, const struct rlimit *rlp);
int getpriority(int which, id_t who);
int setpriority(int which, id_t who, int value);

#ifdef __cplusplus
}
#endif

#endif
