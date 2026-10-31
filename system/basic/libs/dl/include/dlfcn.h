#ifndef _DLFCN_H
#define _DLFCN_H

/*
 * POSIX dynamic-loading interface for EwokOS.
 *
 * EwokOS links every program statically, so there is no ld.so and no
 * interpreter: this header describes a *plugin* facility. dlopen() loads an
 * ET_DYN shared object built with -fPIC -shared, relocates it against the
 * host's exported ABI (see dl/dl.h), and hands back an opaque handle.
 *
 * Two deliberate deviations from glibc/musl behaviour:
 *
 *  - RTLD_LAZY is accepted but ignored. Binding is always eager, because
 *    lazy binding needs a per-object PLT resolver stub and a writable
 *    trampoline, which buys nothing on a system where the whole image is
 *    already resident. A .so with an unresolvable symbol therefore fails at
 *    dlopen() rather than at first call - which is what every caller here
 *    wants anyway.
 *  - RTLD_NEXT is not supported by dlsym(); RTLD_DEFAULT is, and searches
 *    the global scope (host ABI first, then RTLD_GLOBAL objects).
 */

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* mode flags for dlopen() */
#define RTLD_LAZY       0x00001
#define RTLD_NOW        0x00002
#define RTLD_NOLOAD     0x00004
#define RTLD_GLOBAL     0x00100
#define RTLD_LOCAL      0x00000
#define RTLD_NODELETE   0x01000

/* pseudo-handles for dlsym() */
#define RTLD_DEFAULT    ((void*)0)
#define RTLD_NEXT       ((void*)-1)

/* Longest error string dlerror() will report. Sized for a full rootfs path
   plus a symbol name plus an explanation, since that is what the interesting
   failures ("cannot find dependency", "undefined symbol") have to say. */
#define DL_ERRBUF_LEN   512

typedef struct {
	const char* dli_fname;  /* path the object was loaded from */
	void*       dli_fbase;  /* load base (bias) of that object */
	const char* dli_sname;  /* name of the nearest symbol at or below addr */
	void*       dli_saddr;  /* exact address of that symbol */
} Dl_info;

/*
 * Load a shared object and return a handle, or NULL with dlerror() set.
 * A NULL filename returns a handle for the host program's own ABI scope, so
 * dlsym(handle, ...) can look host symbols up. Repeated calls for the same
 * soname return the same handle with the reference count bumped.
 */
void* dlopen(const char* filename, int flags);

/* Drop one reference; unloads the object when the last one goes. */
int   dlclose(void* handle);

/* Resolve a symbol. Returns NULL when not found (check dlerror()). */
void* dlsym(void* handle, const char* symbol);

/*
 * Most recent error, cleared on each successful call and by being read.
 *
 * Deviation from POSIX: this is per-process, not per-thread. Two threads
 * failing concurrently can therefore see each other's message. See the comment
 * on the buffer in libs/dl/src/dl.c for why - the short version is that a
 * per-thread buffer would put a TLS lookup and an allocation inside the path
 * every plugin relocation runs through.
 */
char* dlerror(void);

/* Locate the object containing addr. Returns 0 on success, non-zero on failure. */
int   dladdr(const void* addr, Dl_info* info);

#ifdef __cplusplus
}
#endif

#endif /* _DLFCN_H */
