/*
 * Object graph, lifecycle and the POSIX entry points of the EwokOS loader.
 *
 * There is no ld.so here. dlopen() is the only way an object gets loaded, so
 * the graph is exactly the set of live dlopen handles plus their DT_NEEDED
 * closures, and it is small enough to keep as an intrusive list rather than a
 * hash table.
 *
 * Locking: one mutex serialises every entry point. The loader mutates a global
 * list, a global-scope array and per-object refcounts, and EwokOS has real
 * preemptive pthreads, so an unguarded dlopen() from two threads would corrupt
 * the list. The lock is held across the whole load (file read, map, relocate,
 * init), which means a plugin's constructor must not dlopen() - that would
 * self-deadlock on a non-recursive mutex. This is the same restriction glibc
 * documents for LD_PRELOAD constructors and no realistic plugin hits it.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include <ewoksys/vfs.h>
#include <ewoksys/fsinfo.h>

#include "dl_internal.h"

/* Name of the synthetic object that stands in for the main executable. Chosen
   so it cannot collide with a real soname. */
#define DL_HOST_SONAME   "<host>"

/*
 * Where a bare soname is looked for when $LD_LIBRARY_PATH does not have it.
 * /lib is where the build system installs .so files (see dl/Makefile and the
 * rootfs/lib staging in machine.virt/system/Makefile).
 */
#define DL_DEFAULT_DIRS  "/lib:/usr/lib:/system/lib"

#define DL_ENV_DIRS      "LD_LIBRARY_PATH"

typedef void (*dl_void_fn)(void);

/* ------------------------------------------------------------------ */
/* state                                                               */
/* ------------------------------------------------------------------ */

static pthread_mutex_t dl_mutex = PTHREAD_MUTEX_INITIALIZER;

/* every loaded object, most recently linked first */
static dl_object_t* dl_objects   = NULL;
static uint32_t     dl_nobjects  = 0;

/* objects opened RTLD_GLOBAL, in load order */
static dl_object_t* dl_globals[DL_MAX_GLOBALS];
static uint32_t     dl_nglobals  = 0;

/* the synthetic host object; static storage because it is never unmapped */
static dl_object_t  dl_host;
static int          dl_host_ready = 0;

/*
 * POSIX makes dlerror() per-thread. This is per-process, deliberately: a
 * pthread key here would make the error path allocate and would put a TLS
 * lookup inside the loader that every plugin's relocations run through, at a
 * point in startup where the thread structures may not be live yet. The
 * practical consequence is that two threads failing concurrently can see each
 * other's message. Every error is still set under the loader lock, so a message is
 * never torn.
 */
static char dl_errbuf[DL_ERRBUF_LEN];
static int  dl_err_set = 0;

/* Exposed so dl_abi.c can publish/unpublish under the same lock without
   reaching into this file's statics. */
void dl_lock(void) {
	pthread_mutex_lock(&dl_mutex);
}

void dl_unlock(void) {
	pthread_mutex_unlock(&dl_mutex);
}

void dl_set_error(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(dl_errbuf, sizeof(dl_errbuf), fmt, ap);
	va_end(ap);
	dl_errbuf[sizeof(dl_errbuf) - 1] = 0;
	dl_err_set = 1;
}

void dl_clear_error(void) {
	dl_errbuf[0] = 0;
	dl_err_set = 0;
}

char* dlerror(void) {
	if(!dl_err_set)
		return NULL;
	/* POSIX: reading the error clears it. The buffer itself stays valid until
	   the next call that sets an error, which is the contract callers rely on. */
	dl_err_set = 0;
	return dl_errbuf;
}

/* ------------------------------------------------------------------ */
/* host object                                                         */
/* ------------------------------------------------------------------ */

dl_object_t* dl_host_object(void) {
	if(!dl_host_ready) {
		memset(&dl_host, 0, sizeof(dl_host));
		dl_host.is_host  = 1;
		/* pinned so no refcount arithmetic can ever unmap the host */
		dl_host.refcount = 1;
		dl_host.is64     = (sizeof(void*) == 8) ? 1 : 0;
		dl_host.relocated= 1;
		dl_host.inited   = 1;
		dl_strlcpy(dl_host.soname, DL_HOST_SONAME, DL_NAME_LEN);
		dl_strlcpy(dl_host.path,   DL_HOST_SONAME, DL_PATH_LEN);
		dl_host_ready = 1;
	}
	return &dl_host;
}

/* ------------------------------------------------------------------ */
/* object list                                                         */
/* ------------------------------------------------------------------ */

static void dl_object_link(dl_object_t* o) {
	o->next = dl_objects;
	dl_objects = o;
	dl_nobjects++;
	if((o->flags & RTLD_GLOBAL) != 0 && dl_nglobals < DL_MAX_GLOBALS)
		dl_globals[dl_nglobals++] = o;
}

void dl_object_unlink(dl_object_t* o) {
	dl_object_t** pp = &dl_objects;
	uint32_t i, j;

	while(*pp != NULL) {
		if(*pp == o) {
			*pp = o->next;
			o->next = NULL;
			if(dl_nobjects > 0)
				dl_nobjects--;
			break;
		}
		pp = &(*pp)->next;
	}

	/* drop it from the global scope, packing the rest down */
	for(i = 0, j = 0; i < dl_nglobals; i++) {
		if(dl_globals[i] != o)
			dl_globals[j++] = dl_globals[i];
		else
			dl_globals[i] = NULL;
	}
	dl_nglobals = j;
}

int dl_object_count(void) {
	int n;
	pthread_mutex_lock(&dl_mutex);
	n = (int)dl_nobjects;
	pthread_mutex_unlock(&dl_mutex);
	return n;
}

static int dl_object_is_valid(dl_object_t* o) {
	dl_object_t* p;
	if(o == NULL)
		return 0;
	if(dl_host_ready && o == &dl_host)
		return 1;
	for(p = dl_objects; p != NULL; p = p->next)
		if(p == o)
			return 1;
	return 0;
}

dl_object_t* dl_object_find_by_name(const char* name) {
	dl_object_t* o;
	int has_slash;

	if(name == NULL || name[0] == 0)
		return NULL;
	has_slash = (strchr(name, '/') != NULL) ? 1 : 0;

	for(o = dl_objects; o != NULL; o = o->next) {
		if(strcmp(o->path, name) == 0)
			return o;
		if(o->soname[0] != 0 && strcmp(o->soname, name) == 0)
			return o;
		if(!has_slash) {
			/*
			 * A bare name is a soname query, so matching the tail of the path
			 * it was loaded from is legitimate. A name containing '/' is not:
			 * "/tmp/libfoo.so" must not resolve to an already-loaded
			 * "/lib/libfoo.so" just because the basenames agree.
			 */
			const char* slash = strrchr(o->path, '/');
			const char* tail = (slash != NULL) ? slash + 1 : o->path;
			if(strcmp(tail, name) == 0)
				return o;
		}
	}
	return NULL;
}

dl_object_t* dl_object_find_by_addr(const void* addr) {
	uintptr_t v = (uintptr_t)addr;
	dl_object_t* o;

	for(o = dl_objects; o != NULL; o = o->next) {
		if(o->base == NULL || o->span == 0)
			continue;
		if(v >= (uintptr_t)o->base && v < (uintptr_t)o->base + o->span)
			return o;
	}
	return NULL;
}

/* ------------------------------------------------------------------ */
/* init / fini                                                         */
/* ------------------------------------------------------------------ */

static void dl_run_init(dl_object_t* o) {
	uint32_t i, n;

	if(o == NULL || o->is_host || o->inited)
		return;
	/*
	 * Set the flag before running anything: a constructor that calls
	 * dlopen() on its own object (directly or through a dependency) must
	 * find it already-initialised rather than recurse.
	 */
	o->inited = 1;

	/*
	 * DT_INIT first, then DT_INIT_ARRAY - that is glibc's order and the one
	 * the C++ ABI assumes, since a translation unit's static constructors
	 * land in .init_array while the crt's _init goes in .init.
	 *
	 * EwokOS's own _start never runs .init_array for the host image, so this
	 * is the only place a plugin's constructors get called.
	 */
	if(o->dyn.init != 0 && dl_span_ok(o, o->base + o->dyn.init, 4))
		((dl_void_fn)(uintptr_t)(o->base + o->dyn.init))();

	if(o->dyn.init_array != 0 && o->dyn.init_arraysz >= sizeof(uintptr_t)) {
		uintptr_t* arr = (uintptr_t*)(o->base + o->dyn.init_array);
		n = (uint32_t)(o->dyn.init_arraysz / sizeof(uintptr_t));
		if(dl_span_ok(o, arr, (uint64_t)n * sizeof(uintptr_t))) {
			for(i = 0; i < n; i++) {
				/* 0 and -1 are the two "no function here" markers the ABI
				   allows a linker to leave in the array */
				if(arr[i] == 0 || arr[i] == ~(uintptr_t)0)
					continue;
				((dl_void_fn)(uintptr_t)arr[i])();
			}
		}
	}
}

static void dl_run_fini(dl_object_t* o) {
	uint32_t i, n;

	if(o == NULL || o->is_host || o->finishing || !o->inited)
		return;
	o->finishing = 1;

	if(o->dyn.fini_array != 0 && o->dyn.fini_arraysz >= sizeof(uintptr_t)) {
		uintptr_t* arr = (uintptr_t*)(o->base + o->dyn.fini_array);
		n = (uint32_t)(o->dyn.fini_arraysz / sizeof(uintptr_t));
		if(dl_span_ok(o, arr, (uint64_t)n * sizeof(uintptr_t))) {
			/* reverse of init order: last registered destructs first */
			for(i = n; i > 0; i--) {
				uintptr_t f = arr[i - 1];
				if(f == 0 || f == ~(uintptr_t)0)
					continue;
				((dl_void_fn)(uintptr_t)f)();
			}
		}
	}

	if(o->dyn.fini != 0 && dl_span_ok(o, o->base + o->dyn.fini, 4))
		((dl_void_fn)(uintptr_t)(o->base + o->dyn.fini))();
}

void dl_object_release(dl_object_t* o) {
	uint32_t i;

	if(o == NULL || o->is_host)
		return;
	if(o->refcount > 0)
		o->refcount--;
	if(o->refcount > 0)
		return;

	if((o->flags & RTLD_NODELETE) != 0) {
		/*
		 * POSIX: the object stays resident even at zero references. Re-pin it
		 * so a stray extra release cannot unload it later, and leave it on the
		 * list so the next dlopen() of the same soname reuses the mapping.
		 */
		o->refcount = 1;
		return;
	}

	dl_object_unlink(o);
	dl_run_fini(o);
	/*
	 * Release dependencies after our own fini: init ran deps-first, so fini
	 * must run dependent-first. Each dep carries its own refcount, so one
	 * shared by another live object survives.
	 */
	for(i = 0; i < o->deps_n; i++)
		dl_object_release(o->deps[i]);
	o->deps_n = 0;
	dl_elf_unmap(o);
}

/* ------------------------------------------------------------------ */
/* symbol scope                                                        */
/* ------------------------------------------------------------------ */

static int scope_has(dl_object_t** v, uint32_t n, dl_object_t* o) {
	uint32_t i;
	for(i = 0; i < n; i++)
		if(v[i] == o)
			return 1;
	return 0;
}

/*
 * Build the symbol-search order for `requester`.
 *
 * The ELF rule is: the executable first, then the requesting object, then its
 * DT_NEEDED closure breadth-first. On EwokOS the "executable" is the synthetic
 * host object, whose symbols come from the static ABI table and the runtime
 * registry in dl_abi.c - see dl/dl.h for why --export-dynamic cannot be used
 * instead.
 *
 * DT_SYMBOLIC swaps the first two: such an object binds to its own definitions
 * ahead of everything, including the host. That is the entire point of the tag
 * and a plugin using it to keep a private copy of a symbol would otherwise be
 * silently interposed.
 *
 * Objects opened RTLD_GLOBAL follow, in load order. Objects opened RTLD_LOCAL
 * are deliberately absent: that is what LOCAL means, and a plugin needing
 * another plugin's symbols should be linked against it so it appears in the
 * DT_NEEDED closure instead.
 */
uint32_t dl_build_scope(dl_object_t* requester, dl_object_t** out, uint32_t cap) {
	dl_object_t* q[DL_SEARCH_MAX];
	uint32_t qn = 0, qi = 0, n = 0;
	uint32_t i;
	int symbolic;

	if(cap == 0)
		return 0;
	symbolic = (requester != NULL && requester->dyn.symbolic) ? 1 : 0;

	/* BFS over the requester's local closure */
	if(requester != NULL && qn < DL_SEARCH_MAX)
		q[qn++] = requester;
	while(qi < qn) {
		dl_object_t* cur = q[qi++];
		uint32_t k;
		for(k = 0; k < cur->deps_n; k++) {
			dl_object_t* d = cur->deps[k];
			if(d == NULL || d->is_host)
				continue;
			if(scope_has(q, qn, d))
				continue;
			if(qn >= DL_SEARCH_MAX)
				break;
			q[qn++] = d;
		}
	}

	if(symbolic) {
		for(qi = 0; qi < qn && n < cap; qi++)
			out[n++] = q[qi];
		if(n < cap)
			out[n++] = dl_host_object();
	} else {
		out[n++] = dl_host_object();
		for(qi = 0; qi < qn && n < cap; qi++)
			out[n++] = q[qi];
	}

	for(i = 0; i < dl_nglobals && n < cap; i++) {
		dl_object_t* g = dl_globals[i];
		if(g == NULL || scope_has(out, n, g))
			continue;
		out[n++] = g;
	}
	return n;
}

uintptr_t dl_lookup_symbol(const char* name, dl_object_t* requester,
                           dl_object_t** defobj) {
	dl_object_t* scope[DL_SEARCH_MAX];
	uint32_t n, i;

	if(defobj != NULL)
		*defobj = NULL;
	if(name == NULL || name[0] == 0)
		return 0;

	n = dl_build_scope(requester, scope, DL_SEARCH_MAX);
	for(i = 0; i < n; i++) {
		dl_object_t* o = scope[i];
		int idx;
		dl_sym_t s;

		if(o->is_host) {
			void* p = dl_host_abi_lookup(name);
			if(p != NULL) {
				if(defobj != NULL)
					*defobj = o;
				return (uintptr_t)p;
			}
			continue;
		}

		idx = dl_hash_lookup(o, name);
		if(idx < 0)
			continue;
		if(dl_sym_at(o, (uint32_t)idx, &s) != 0)
			continue;
		/* dl_hash_lookup already skips SHN_UNDEF, but re-check: a stale
		   index must never turn into a jump to address 0 */
		if(s.shndx == SHN_UNDEF)
			continue;
		if(defobj != NULL)
			*defobj = o;
		return (uintptr_t)o->base + (uintptr_t)s.value;
	}
	return 0;
}

/* ------------------------------------------------------------------ */
/* path resolution                                                     */
/* ------------------------------------------------------------------ */

/*
 * Join dir + '/' + name into out, refusing rather than truncating when the
 * result would not fit.
 *
 * snprintf("%s/%s") would silently shorten an over-long join, and the probe
 * that follows would then report "not found" for a directory that was never
 * actually searched. Doing the copy by hand keeps the bound provable - which
 * also matters because GCC's -Wformat-truncation cannot see one through an
 * unbounded const char* and warns on code that is already correct.
 */
static int dl_join(char* out, uint32_t cap, const char* dir, const char* name) {
	uint32_t dl, nl;

	if(out == NULL || cap == 0 || dir == NULL || name == NULL)
		return -1;
	dl = (uint32_t)strlen(dir);
	nl = (uint32_t)strlen(name);
	if(dl == 0 || nl == 0)
		return -1;
	if(dl + 1 + nl + 1 > cap)
		return -1;
	memcpy(out, dir, dl);
	out[dl] = '/';
	memcpy(out + dl + 1, name, nl);
	out[dl + 1 + nl] = 0;
	return 0;
}

static int dl_file_is_regular(const char* path) {
	fsinfo_t info;
	char full[FS_FULL_NAME_MAX + 1];

	if(path == NULL || path[0] == 0)
		return 0;
	memset(&info, 0, sizeof(info));
	vfs_fullname(path, full, FS_FULL_NAME_MAX);
	full[FS_FULL_NAME_MAX] = 0;
	if(vfs_get_by_name(full, &info) != 0)
		return 0;
	return (info.type == FS_TYPE_FILE) ? 1 : 0;
}

/* Walk a colon-separated dir list, trying each as a prefix for `name`. */
static int dl_search_dirs(const char* dirs, const char* name,
                          char* out, uint32_t cap) {
	const char* p = dirs;

	if(dirs == NULL)
		return -1;

	while(*p != 0) {
		const char* colon = strchr(p, ':');
		uint32_t len = (colon != NULL) ? (uint32_t)(colon - p)
		                               : (uint32_t)strlen(p);
		if(len > 0) {
			char d[DL_PATH_LEN];
			char try[DL_PATH_LEN];
			if(len >= DL_PATH_LEN)
				len = DL_PATH_LEN - 1;
			memcpy(d, p, len);
			d[len] = 0;
			/* tolerate both "/lib" and "/lib/" spellings */
			if(len > 0 && d[len - 1] == '/')
				d[len - 1] = 0;
			if(dl_join(try, sizeof(try), d, name) == 0 &&
			   dl_file_is_regular(try)) {
				dl_strlcpy(out, try, cap);
				return 0;
			}
		}
		if(colon == NULL)
			break;
		p = colon + 1;
	}
	return -1;
}

/*
 * Turn `name` into a path that exists, writing it to `out`.
 *
 * A name containing '/' is used verbatim: dlopen("/path/to/x.so") means exactly
 * that file, and silently falling back to /lib would load the wrong object.
 *
 * A bare name is a soname and gets searched. `ref` (may be NULL) is the path of
 * the object asking, and its own directory is tried first - that is how a plugin
 * sitting in /lib finds its sibling without an rpath, which the EwokOS rootfs
 * has no place to put.
 */
static int dl_resolve_path(const char* name, const char* ref,
                           char* out, uint32_t cap) {
	if(name == NULL || name[0] == 0 || out == NULL || cap == 0)
		return -1;

	if(strchr(name, '/') != NULL) {
		if(dl_file_is_regular(name)) {
			dl_strlcpy(out, name, cap);
			return 0;
		}
		return -1;
	}

	if(ref != NULL) {
		const char* slash = strrchr(ref, '/');
		if(slash != NULL && slash != ref) {
			char d[DL_PATH_LEN];
			char try[DL_PATH_LEN];
			uint32_t len = (uint32_t)(slash - ref);
			if(len >= DL_PATH_LEN)
				len = DL_PATH_LEN - 1;
			memcpy(d, ref, len);
			d[len] = 0;
			if(dl_join(try, sizeof(try), d, name) == 0 &&
			   dl_file_is_regular(try)) {
				dl_strlcpy(out, try, cap);
				return 0;
			}
		}
	}

	if(dl_search_dirs(getenv(DL_ENV_DIRS), name, out, cap) == 0)
		return 0;
	if(dl_search_dirs(DL_DEFAULT_DIRS, name, out, cap) == 0)
		return 0;
	return -1;
}

/* ------------------------------------------------------------------ */
/* loading                                                             */
/* ------------------------------------------------------------------ */

dl_object_t* dl_load_from_path(const char* path, int flags, int depth) {
	uint8_t* img;
	int fsize = 0;
	dl_object_t* o;
	dl_object_t* existing;
	uint32_t i;

	if(path == NULL || path[0] == 0) {
		dl_set_error("empty object path");
		return NULL;
	}
	if(depth > DL_MAX_DEPTH) {
		dl_set_error("%s: the DT_NEEDED chain is deeper than %d levels, which "
		             "means a dependency cycle", path, DL_MAX_DEPTH);
		return NULL;
	}

	existing = dl_object_find_by_name(path);
	if(existing != NULL) {
		existing->refcount++;
		/* a later RTLD_GLOBAL widens an object first opened LOCAL, matching
		   glibc: visibility is a property of the object, not of one handle */
		if((flags & RTLD_GLOBAL) != 0 &&
		   (existing->flags & RTLD_GLOBAL) == 0) {
			existing->flags |= RTLD_GLOBAL;
			if(dl_nglobals < DL_MAX_GLOBALS)
				dl_globals[dl_nglobals++] = existing;
		}
		return existing;
	}

	img = vfs_readfile(path, &fsize);
	if(img == NULL || fsize <= 0) {
		if(img != NULL)
			free(img);
		dl_set_error("%s: cannot read the file - is it present on this "
		             "filesystem, and a regular file?", path);
		return NULL;
	}

	o = dl_elf_map(img, (uint32_t)fsize, path);
	/*
	 * dl_elf_map copies every PT_LOAD segment into its own mapping, so the file
	 * buffer is dead from here on. Releasing it immediately matters: a plugin
	 * is read into a second full copy of itself otherwise, and EwokOS heap is
	 * a fixed pre-allocated region.
	 */
	free(img);
	img = NULL;
	if(o == NULL)
		return NULL;

	o->flags    = flags;
	o->refcount = 1;

	/*
	 * Link it in before walking DT_NEEDED. A dependency cycle then resolves to
	 * the already-linked object through dl_object_find_by_name() instead of
	 * recursing until DL_MAX_DEPTH, and a dependency's own relocation can see
	 * this object if it refers back.
	 */
	dl_object_link(o);

	for(i = 0; i < o->dyn.needed_n; i++) {
		const char* need = dl_dyn_string(o, o->dyn.needed[i]);
		char full[DL_PATH_LEN];
		dl_object_t* dep;

		if(need == NULL || need[0] == 0)
			continue;
		if(dl_resolve_path(need, o->path, full, sizeof(full)) != 0) {
			dl_set_error("%s: cannot find dependency '%s' (searched the "
			             "object's own directory, $%s and %s)",
			             path, need, DL_ENV_DIRS, DL_DEFAULT_DIRS);
			goto fail;
		}
		/* a dependency inherits this object's visibility but not NOLOAD/NOW,
		   which are properties of the dlopen call, not of the graph */
		dep = dl_load_from_path(full, flags & RTLD_GLOBAL, depth + 1);
		if(dep == NULL)
			goto fail;
		if(o->deps_n < DL_MAX_NEEDED) {
			o->deps[o->deps_n++] = dep;
		} else {
			dl_object_release(dep);
			dl_set_error("%s: more than %d dependencies", path, DL_MAX_NEEDED);
			goto fail;
		}
	}

	/* dependencies are already relocated, which is what makes this object's
	   own symbols resolvable against them */
	if(dl_relocate(o) != 0)
		goto fail;

	/* the segment bytes arrived as data stores; publish them to instruction
	   fetch before anything calls into the object */
	dl_cache_sync(o);

	/*
	 * Now that the relocations are applied and the code is published, drop the
	 * write permission off the executable pages (W^X). This must run after
	 * dl_relocate() - which writes into the data pages - and before dl_run_init()
	 * - which executes the constructors. On a kernel with the anonymous window
	 * this is real; on the heap fallback mprotect() is a no-op and it succeeds
	 * without changing anything.
	 */
	if(dl_elf_protect(o) != 0)
		goto fail;

	/*
	 * Dependencies were relocated and initialised on the way in, so running this
	 * object's own init now gives the required deps-before-dependent order
	 * without a separate topological pass.
	 */
	dl_run_init(o);
	return o;

fail:
	{
		uint32_t k;
		dl_object_unlink(o);
		for(k = 0; k < o->deps_n; k++)
			dl_object_release(o->deps[k]);
		o->deps_n = 0;
		dl_elf_unmap(o);
	}
	return NULL;
}

/* ------------------------------------------------------------------ */
/* POSIX entry points                                                  */
/* ------------------------------------------------------------------ */

void* dlopen(const char* filename, int flags) {
	dl_object_t* o;
	char full[DL_PATH_LEN];
	void* ret;

	pthread_mutex_lock(&dl_mutex);
	dl_clear_error();

	/* NULL filename: a handle for the host's own ABI scope, so that
	   dlsym(handle, ...) can reach host symbols without naming an object */
	if(filename == NULL) {
		o = dl_host_object();
		pthread_mutex_unlock(&dl_mutex);
		return (void*)o;
	}

	if((flags & RTLD_NOLOAD) != 0) {
		o = dl_object_find_by_name(filename);
		/* POSIX: RTLD_NOLOAD on an object that is not loaded returns NULL
		   without setting an error - it is a query, not a request */
		if(o != NULL)
			o->refcount++;
		pthread_mutex_unlock(&dl_mutex);
		return (o != NULL) ? (void*)o : NULL;
	}

	/* RTLD_LAZY is accepted and ignored; binding is always eager (dlfcn.h) */

	if(dl_resolve_path(filename, NULL, full, sizeof(full)) != 0) {
		if(strchr(filename, '/') != NULL)
			dl_set_error("%s: no such file", filename);
		else
			dl_set_error("%s: not found in $%s or %s",
			             filename, DL_ENV_DIRS, DL_DEFAULT_DIRS);
		pthread_mutex_unlock(&dl_mutex);
		return NULL;
	}

	o = dl_load_from_path(full, flags, 0);
	ret = (o != NULL) ? (void*)o : NULL;
	pthread_mutex_unlock(&dl_mutex);
	return ret;
}

int dlclose(void* handle) {
	dl_object_t* o = (dl_object_t*)handle;

	if(handle == NULL)
		return 0;

	pthread_mutex_lock(&dl_mutex);
	dl_clear_error();

	if(!dl_object_is_valid(o)) {
		dl_set_error("dlclose: %p is not a handle that dlopen returned", handle);
		pthread_mutex_unlock(&dl_mutex);
		return -1;
	}
	/* the host is pinned; dropping a reference to it is a no-op, not an error */
	if(!o->is_host)
		dl_object_release(o);

	pthread_mutex_unlock(&dl_mutex);
	return 0;
}

void* dlsym(void* handle, const char* symbol) {
	dl_object_t* o = (dl_object_t*)handle;
	void* ret = NULL;

	pthread_mutex_lock(&dl_mutex);
	dl_clear_error();

	if(symbol == NULL || symbol[0] == 0) {
		dl_set_error("dlsym: NULL or empty symbol name");
		pthread_mutex_unlock(&dl_mutex);
		return NULL;
	}

	if(handle == RTLD_NEXT) {
		dl_set_error("dlsym: RTLD_NEXT is not supported on EwokOS. There is no "
		             "interposition chain to walk, because the host is a "
		             "static link and cannot be overridden by a plugin "
		             "(see the note in dlfcn.h)");
		pthread_mutex_unlock(&dl_mutex);
		return NULL;
	}

	if(handle == RTLD_DEFAULT) {
		/* global scope: host ABI, then RTLD_GLOBAL objects in load order */
		ret = (void*)(uintptr_t)dl_lookup_symbol(symbol, NULL, NULL);
	} else if(!dl_object_is_valid(o)) {
		dl_set_error("dlsym: %p is not a handle that dlopen returned", handle);
	} else if(o->is_host) {
		ret = dl_host_abi_lookup(symbol);
	} else {
		int idx = dl_hash_lookup(o, symbol);
		if(idx >= 0) {
			dl_sym_t s;
			if(dl_sym_at(o, (uint32_t)idx, &s) == 0 && s.shndx != SHN_UNDEF)
				ret = (void*)(uintptr_t)(o->base + (uintptr_t)s.value);
		}
	}

	/* a NULL return with no message set means "looked everywhere, absent" */
	if(ret == NULL && !dl_err_set)
		dl_set_error("%s: symbol '%s' not found",
		             (o != NULL && !o->is_host && dl_object_is_valid(o))
		                 ? o->path : DL_HOST_SONAME,
		             symbol);

	pthread_mutex_unlock(&dl_mutex);
	return ret;
}

int dladdr(const void* addr, Dl_info* info) {
	dl_object_t* o;
	int ret = 0;

	if(info == NULL)
		return 0;
	memset(info, 0, sizeof(*info));

	pthread_mutex_lock(&dl_mutex);
	dl_clear_error();

	o = dl_object_find_by_addr(addr);
	if(o != NULL) {
		dl_sym_t s;
		info->dli_fname = o->path;
		info->dli_fbase = (void*)o->base;
		if(dl_sym_find_nearest(o, (uintptr_t)addr, &s) >= 0) {
			info->dli_sname = dl_sym_name(o, &s);
			info->dli_saddr = (void*)(uintptr_t)(o->base + (uintptr_t)s.value);
		}
		ret = 1;
	} else {
		/*
		 * Not an error worth failing quietly over: an EwokOS program is a
		 * static ET_EXEC, so a host code address lies in no shared object and
		 * there is nothing to report. Say so, because the caller (a profiler
		 * or a crash handler) usually wants to know why it got 0.
		 */
		dl_set_error("dladdr: %p is not inside any loaded shared object "
		             "(EwokOS host programs are statically linked, so host "
		             "code addresses belong to no object)", addr);
	}

	pthread_mutex_unlock(&dl_mutex);
	return ret;
}
