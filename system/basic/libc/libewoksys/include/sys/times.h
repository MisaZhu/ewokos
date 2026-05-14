#ifndef EWOKOS_LIBC_SYS_TIMES_H
#define EWOKOS_LIBC_SYS_TIMES_H

#include <sys/types.h>

struct tms {
	clock_t tms_utime;
	clock_t tms_stime;
	clock_t tms_cutime;
	clock_t tms_cstime;
};

clock_t times(struct tms *buf);

#endif
