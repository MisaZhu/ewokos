#ifndef EWOKOS_GLOB_H
#define EWOKOS_GLOB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	size_t gl_pathc;    /* count of matched paths */
	char **gl_pathv;    /* list of matched paths */
	size_t gl_offs;     /* slots to reserve in gl_pathv */
} glob_t;

#define GLOB_ERR      (1 << 0)
#define GLOB_MARK     (1 << 1)
#define GLOB_NOSORT   (1 << 2)
#define GLOB_DOOFFS   (1 << 3)
#define GLOB_NOCHECK  (1 << 4)
#define GLOB_APPEND   (1 << 5)
#define GLOB_NOESCAPE (1 << 6)
#define GLOB_PERIOD   (1 << 7)

#define GLOB_NOSPACE  (-1)
#define GLOB_ABORTED  (-2)
#define GLOB_NOMATCH  (-3)

int glob(const char *pattern, int flags,
		int (*errfunc)(const char *epath, int eerrno), glob_t *pglob);
void globfree(glob_t *pglob);

#ifdef __cplusplus
}
#endif

#endif
