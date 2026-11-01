/*
 * Host symbol publication and lookup.
 *
 * Two tables and one registry, searched in this order:
 *
 *   runtime registry   - what the application published with dl_host_publish()
 *   core table         - libdl's own entry points (below)
 *   generated table    - every global symbol of the EWOK_LIBC archives
 *
 * The generated table is dl_export.c, produced at build time by gen_export.sh.
 * See dl/dl.h for why the host has to publish anything at all: a static
 * ET_EXEC link has no .dynsym, so --export-dynamic cannot do the job it does on
 * Linux.
 */

#include <stdlib.h>
#include <string.h>
#include <dl/dl.h>

#include "dl_internal.h"

/*
 * Symbols libdl itself owns.
 *
 * These cannot come from the generated table, which nm's EWOK_LIBC
 * (libewoksys/libc/libgloss); libdl is not part of EWOK_LIBC and could not be,
 * since the table is compiled into libdl.
 *
 * Exporting them matters more than it looks. A plugin that loads a second
 * plugin, or that calls dl_host_publish() to hand a callback back to its host,
 * needs the loader's own entry points - and unlike libc, there is no other copy
 * of them anywhere in the link.
 *
 * (uintptr_t)(void*)&f is the portable spelling here: the C standard does not
 * guarantee a function pointer converts to an integer, but POSIX requires it for
 * exactly this dlsym-style use, and every compiler EwokOS builds with does it.
 */
#define DL_CORE_SYM(f)  { #f, (uintptr_t)(void*)&f }

/* INVARIANT: sorted by strcmp(name). table_find() binary searches it, and the
   generated table carries the same requirement - see dl/dl.h. */
static const dl_symbol_t dl_abi_core[] = {
	DL_CORE_SYM(dl_host_abi_version),
	DL_CORE_SYM(dl_host_lookup),
	DL_CORE_SYM(dl_host_publish),
	DL_CORE_SYM(dl_host_unpublish),
	DL_CORE_SYM(dl_object_count),
	DL_CORE_SYM(dladdr),
	DL_CORE_SYM(dlclose),
	DL_CORE_SYM(dlerror),
	DL_CORE_SYM(dlopen),
	DL_CORE_SYM(dlsym),
};

#define DL_CORE_COUNT \
	((uint32_t)(sizeof(dl_abi_core) / sizeof(dl_abi_core[0])))

/*
 * Runtime registry. Fixed size, insertion ordered: unpublish packs down rather
 * than leaving holes, so a linear scan over at most DL_HOST_SYMS entries stays
 * contiguous and needs no allocation. An application publishes a handful of
 * callbacks, not thousands, so this is not the hot path - relocations resolve
 * overwhelmingly against the generated table.
 */
static dl_symbol_t dl_registry[DL_HOST_SYMS];
static uint32_t    dl_registry_n = 0;

uint32_t dl_host_abi_version(void) {
	return EWOK_DL_ABI_VERSION;
}

/* ------------------------------------------------------------------ */
/* lookup                                                              */
/* ------------------------------------------------------------------ */

static void* registry_find(const char* name, uint32_t* idx_out) {
	uint32_t i;
	for(i = 0; i < dl_registry_n; i++) {
		if(dl_registry[i].name != NULL &&
		   strcmp(dl_registry[i].name, name) == 0) {
			if(idx_out != NULL)
				*idx_out = i;
			return (void*)dl_registry[i].addr;
		}
	}
	if(idx_out != NULL)
		*idx_out = 0;
	return NULL;
}

/* Binary search a table that is sorted by strcmp(name). */
static void* table_find(const dl_symbol_t* tbl, uint32_t n, const char* name) {
	uint32_t lo = 0, hi = n;
	while(lo < hi) {
		uint32_t mid = lo + (hi - lo) / 2;
		int c;
		if(tbl[mid].name == NULL)
			return NULL;        /* a hole means the sort invariant broke */
		c = strcmp(tbl[mid].name, name);
		if(c == 0)
			return (tbl[mid].addr != 0) ? (void*)tbl[mid].addr : NULL;
		if(c < 0)
			lo = mid + 1;
		else
			hi = mid;
	}
	return NULL;
}

void* dl_host_abi_lookup(const char* name) {
	void* p;

	if(name == NULL || name[0] == 0)
		return NULL;

	/*
	 * Registry first. An application that publishes a name the generated table
	 * also holds is overriding it on purpose - typically to give a plugin the
	 * app's own allocator or logging hook rather than libc's.
	 */
	p = registry_find(name, NULL);
	if(p != NULL)
		return p;

	p = table_find(dl_abi_core, DL_CORE_COUNT, name);
	if(p != NULL)
		return p;

	return table_find(__ewok_dl_abi, __ewok_dl_abi_count, name);
}

/* ------------------------------------------------------------------ */
/* publication                                                         */
/* ------------------------------------------------------------------ */

int dl_host_publish(const char* name, void* addr) {
	int ret = 0;

	if(name == NULL || name[0] == 0)
		return -1;
	/*
	 * Refuse a NULL address rather than storing it. table_find() treats 0 as
	 * "not exported", so accepting NULL would create an entry that is present
	 * but unfindable - and unpublishing it would then look like a no-op.
	 */
	if(addr == NULL)
		return -1;

	dl_lock();
	if(registry_find(name, NULL) != NULL) {
		/*
		 * Refuse rather than shadow. A plugin loaded before the second publish
		 * already bound to the old address in its GOT; one loaded after would
		 * bind to the new one. Two behaviours for a single name is a debugging
		 * trap, so the caller has to unpublish first and mean it.
		 */
		ret = -1;
	} else if(dl_registry_n >= DL_HOST_SYMS) {
		ret = -1;
	} else {
		/* the name pointer is NOT copied - see the contract in dl/dl.h */
		dl_registry[dl_registry_n].name = name;
		dl_registry[dl_registry_n].addr = (uintptr_t)addr;
		dl_registry_n++;
	}
	dl_unlock();
	return ret;
}

int dl_host_unpublish(const char* name) {
	uint32_t idx = 0, i;
	int ret = -1;

	if(name == NULL || name[0] == 0)
		return -1;

	dl_lock();
	if(registry_find(name, &idx) != NULL) {
		/* pack down so the registry stays hole-free for registry_find() */
		for(i = idx; i + 1 < dl_registry_n; i++)
			dl_registry[i] = dl_registry[i + 1];
		dl_registry_n--;
		dl_registry[dl_registry_n].name = NULL;
		dl_registry[dl_registry_n].addr = 0;
		ret = 0;
	}
	dl_unlock();
	return ret;
}

void* dl_host_lookup(const char* name) {
	void* p;
	dl_lock();
	p = dl_host_abi_lookup(name);
	dl_unlock();
	return p;
}
