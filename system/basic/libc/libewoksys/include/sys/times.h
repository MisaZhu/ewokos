#ifndef EWOKOS_LIBC_SYS_TIMES_H
#define EWOKOS_LIBC_SYS_TIMES_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct tms {
	clock_t tms_utime;
	clock_t tms_stime;
	clock_t tms_cutime;
	clock_t tms_cstime;
};

clock_t times(struct tms *buf);

#ifdef __cplusplus
}
#endif

#endif
