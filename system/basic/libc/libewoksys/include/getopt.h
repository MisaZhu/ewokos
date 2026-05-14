#ifndef EWOKOS_LIBC_GETOPT_H
#define EWOKOS_LIBC_GETOPT_H

#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

struct option {
	const char *name;
	int has_arg;
	int *flag;
	int val;
};

#define no_argument 0
#define required_argument 1
#define optional_argument 2

int getopt_long(int argc, char * const argv[], const char *optstring,
		const struct option *longopts, int *longindex);

#ifdef __cplusplus
}
#endif

#endif
