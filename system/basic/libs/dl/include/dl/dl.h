#ifndef EWOK_DL_H
#define EWOK_DL_H

/*
 * EwokOS-specific extensions to <dlfcn.h>.
 *
 * The problem this solves: on Linux a plugin's undefined symbols resolve
 * against the main executable because the executable carries a .dynsym that
 * -Wl,-E/--export-dynamic populates. EwokOS programs are fully static
 * ET_EXEC links, and GNU ld creates no dynamic section at all for those, so
 * --export-dynamic is a no-op - there is nothing for a loader to read.
 *
 * The host therefore has to publish its ABI explicitly. Two mechanisms:
 *
 *  1. A build-time generated table (dl_export.c, see gen_export.sh) holding
 *     every global symbol of the archives named by EWOK_LIBC - libewoksys,
 *     libc and libgloss. That is the same surface a Linux plugin would get
 *     from the shared libc, and because the references are strong, linking
 *     libdl into an app pulls exactly those objects in. It is versioned so a
 *     plugin can refuse a host that is too old.
 *  2. dl_host_publish()/dl_host_unpublish() for symbols the application owns
 *     (its own callbacks, its widget tree, its private API), plus anything
 *     from a library outside EWOK_LIBC such as libm. These need no
 *     build-system change, which matters because making the generated table
 *     cover every library would force whole-archive linking of libx, libgraph
 *     and friends into every plugin host.
 *
 * Both land in the same lookup scope, ahead of any loaded object, so a plugin
 * sees the host exactly where it would see the main executable on Linux.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ABI contract version. Bump whenever an entry is REMOVED from the static
 * table or its signature changes; adding entries does not require a bump.
 * A plugin built against a newer ABI can check this at load time rather than
 * dereferencing a NULL it did not expect.
 */
#define EWOK_DL_ABI_VERSION 1

/*
 * The host's ABI version, as a function and not only as a macro: the macro is
 * baked into a plugin at compile time, so testing it tells you what the plugin
 * was BUILT against, not what it is RUNNING against. A plugin that needs a
 * symbol added in version N can therefore refuse to load with a readable
 * message instead of dereferencing a NULL it did not expect. This function is
 * itself in the core export table, so looking it up always works.
 */
uint32_t dl_host_abi_version(void);

/* One published symbol. addr is stored as uintptr_t so the table can be
   emitted as pure data and so a publisher can hand over a non-function
   address (a variable, a vtable) without a cast dance. */
typedef struct {
	const char* name;
	uintptr_t   addr;
} dl_symbol_t;

/*
 * The static host ABI table, defined in the generated dl_export.c. The loader
 * walks [0, __ewok_dl_abi_count).
 *
 * INVARIANT: the array is sorted by strcmp(name). It is produced by a
 * build-time nm pass (see libs/dl/gen_export.sh) that sorts before emitting,
 * and dl_abi.c binary-searches it. A hand-edited table that breaks the order
 * will silently miss symbols rather than fail, so keep the generator as the
 * only source. Kept visible so a host can dump what it exports (see the dltest
 * app) without a dedicated accessor.
 */
extern const dl_symbol_t __ewok_dl_abi[];
extern const uint32_t    __ewok_dl_abi_count;

/*
 * Publish an application-owned symbol at runtime. The name pointer is NOT
 * copied, so it must stay valid for as long as the symbol is published -
 * a string literal is the normal choice. Returns 0 on success, -1 when the
 * registry is full or the name is already published by the application
 * (re-publishing the same name is refused rather than silently shadowed, so a
 * plugin cannot end up bound to a stale pointer).
 */
int dl_host_publish(const char* name, void* addr);

/* Remove a symbol published by dl_host_publish(). Static ABI entries cannot
   be unpublished. Returns 0 on success, -1 when not found. */
int dl_host_unpublish(const char* name);

/*
 * Look a host symbol up directly, without going through a dlopen() handle.
 * Searches the runtime registry first, then the static ABI table. Returns
 * NULL when the host does not export it.
 */
void* dl_host_lookup(const char* name);

/* Number of loaded shared objects (diagnostics / leak checks). */
int dl_object_count(void);

#ifdef __cplusplus
}
#endif

#endif /* EWOK_DL_H */
