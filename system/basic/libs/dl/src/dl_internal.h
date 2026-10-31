#ifndef DL_INTERNAL_H
#define DL_INTERNAL_H

/*
 * Internal interfaces of the EwokOS dynamic loader. Not installed; only the
 * files under src/ include this.
 *
 * Design note on class handling: EwokOS builds ELFCLASS_32 (arm/x86/riscv32)
 * and ELFCLASS_64 (aarch64/x86_64/riscv64) targets from one source tree, so
 * every table walk has to cope with both. Rather than duplicating the loader,
 * the parse step normalizes what it finds into fixed-width fields (uint64_t
 * values, a dl_sym_t / dl_reloc_t view) and the rest of the loader only ever
 * sees the normalized form. The two classes then differ in exactly four
 * places: dl_sym_at(), dl_reloc_at(), dl_parse_dynamic() and the pointer
 * width used when writing a resolved address back.
 */

#include <stdint.h>
#include <stddef.h>
#include <elf/elf.h>
#include <elf/elf_dyn.h>
#include <dlfcn.h>
#include <dl/dl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DL_MAX_OBJECTS   64
#define DL_MAX_NEEDED    16
#define DL_MAX_GLOBALS   32
#define DL_NAME_LEN      128
#define DL_PATH_LEN      256
#define DL_HOST_SYMS     128
/* how deep DT_NEEDED recursion may go before it is treated as a cycle */
#define DL_MAX_DEPTH     16

/*
 * Alignment a .so load base must satisfy.
 *
 * aarch64 -shared output carries p_align = 0x10000 even when the kernel runs
 * 4k or 16k pages (machine.virt builds with PAGE_SIZE=16k). Placing the base
 * on a 64k boundary makes the ELF congruence rule
 *     (base + p_vaddr) mod PAGE_SIZE == p_offset mod PAGE_SIZE
 * hold for every page size EwokOS supports, and keeps each PT_LOAD segment's
 * permission bits describable by whole pages - which is what Phase 6's
 * mprotect pass will need.
 */
#define DL_BASE_ALIGN    0x10000u

/* Cap on PT_LOAD segments recorded for the W^X permission pass. A -shared
   object has two in practice (RX text, RW data); eight is generous headroom,
   and dl_elf_map refuses an object with more rather than silently dropping a
   segment it could then leave writable-and-executable. */
#define DL_MAX_LOAD_SEGS 8

/* relocation table selectors for dl_reloc_at() / dl_reloc_count() */
enum {
	DL_RT_RELA = 0,   /* DT_RELA  */
	DL_RT_REL,        /* DT_REL   */
	DL_RT_PLT         /* DT_JMPREL, class implied by DT_PLTREL */
};

/* Normalized PT_DYNAMIC contents. Every address-valued field holds the
   UNBIASED value exactly as it appears in the file (i.e. relative to a load
   base of 0); callers add o->base. Sizes are byte counts. */
typedef struct {
	uint64_t strtab, strsz;
	uint64_t symtab, syment;
	uint64_t hash;
	uint64_t rela, relasz, relaent;
	uint64_t rel, relsz, relent;
	uint64_t jmprel, pltrelsz, pltrel;
	uint64_t init, fini;
	uint64_t init_array, init_arraysz;
	uint64_t fini_array, fini_arraysz;
	uint64_t soname;                  /* string-table offset, 0 if absent */
	uint64_t needed[DL_MAX_NEEDED];   /* string-table offsets */
	uint32_t needed_n;
	int      symbolic;                /* DT_SYMBOLIC present */
	int      textrel;                 /* DT_TEXTREL: relocs write to a RX page */
	int      bind_now;                /* DT_BIND_NOW present */
	int      gnu_hash;                /* DT_GNU_HASH present (unsupported) */
	int      has_tls;                 /* PT_TLS present (unsupported) */
} dl_dynamic_t;

/* Normalized .dynsym entry */
typedef struct {
	uint64_t value;      /* unbiased st_value */
	uint64_t size;
	uint32_t name_off;   /* offset into strtab */
	uint16_t shndx;
	uint8_t  bind;       /* STB_* */
	uint8_t  type;       /* STT_* */
} dl_sym_t;

/* Normalized relocation entry */
typedef struct {
	uint64_t offset;     /* unbiased r_offset */
	uint64_t addend;     /* RELA: r_addend; REL: the in-place word */
	uint32_t symidx;
	uint32_t type;
	int      has_addend; /* RELA (1) vs REL (0) - REL reads the target word */
} dl_reloc_t;

typedef struct dl_object {
	struct dl_object* next;

	char      soname[DL_NAME_LEN];
	char      path[DL_PATH_LEN];

	uint8_t*  raw_alloc;  /* exactly what munmap() must get back */
	uint8_t*  base;       /* load bias; every unbiased VA plus this */
	uint32_t  span;       /* bytes mapped from base */
	uint32_t  alloc_span; /* bytes requested from mmap (>= span + slack) */

	int       is64;
	int       refcount;
	int       flags;      /* the dlopen flags this object was opened with */
	int       relocated;  /* relocations applied */
	int       inited;     /* DT_INIT / DT_INIT_ARRAY already run */
	int       finishing;  /* guards re-entrant dlclose during fini */
	int       is_host;    /* synthetic object standing for the host ABI */
	int       on_stack;   /* cycle guard for the dependency walk */

	/*
	 * Location of the PT_DYNAMIC array, kept as a pointer into the FILE
	 * image rather than into the mapped copy: the parse happens before the
	 * segments are in place, and every DT_* address value is unbiased
	 * anyway, so reading it from the file avoids depending on the copy
	 * having succeeded.
	 */
	const uint8_t* dyn_img;
	uint32_t       dyn_bytes;
	uint64_t       load_lo;   /* lowest PT_LOAD vaddr, alignment-masked */

	/*
	 * PT_LOAD segments, recorded during dl_elf_map's copy pass as offsets
	 * from o->base with their p_flags, so dl_elf_protect() can re-permission
	 * each one after the relocations are applied: text to read-only-and-
	 * execute, data to read-write, and the padding between segments to
	 * no-access. That is the W^X the dedicated window exists to give; the
	 * old single RWX mapping could not express it.
	 */
	struct {
		uint64_t vbase;   /* segment start relative to o->base */
		uint64_t memsz;
		uint32_t flags;   /* PF_R|PF_W|PF_X */
	} segs[DL_MAX_LOAD_SEGS];
	uint32_t       segs_n;

	dl_dynamic_t dyn;

	struct dl_object* deps[DL_MAX_NEEDED];
	uint32_t          deps_n;
} dl_object_t;

/* --- dl.c --- */
/* Serialise every loader entry point. The mutex is NON-recursive, so code that
   already runs under it (relocation, init/fini, dl_lookup_symbol) must use the
   lock-free internal entry points - dl_host_abi_lookup() rather than
   dl_host_lookup() - or it will deadlock against itself. */
void dl_lock(void);
void dl_unlock(void);

void  dl_set_error(const char* fmt, ...);
void  dl_clear_error(void);

dl_object_t* dl_object_find_by_name(const char* name);
dl_object_t* dl_object_find_by_addr(const void* addr);
dl_object_t* dl_host_object(void);

/* Load path, soname and dependencies. Returns NULL with dlerror() set.
   depth bounds DT_NEEDED recursion; caller holds the loader lock. */
dl_object_t* dl_load_from_path(const char* path, int flags, int depth);

/* --- dl_elf.c --- */
/* Bounded, always-terminating string copy (libgloss's strncpy pads). */
void dl_strlcpy(char* dst, const char* src, uint32_t cap);

/* Parse and map the image at `img` (fsize bytes) into a fresh dl_object_t.
   Fills in the PT_DYNAMIC view and the segments, but does NOT resolve
   dependencies or apply relocations. Returns NULL with dlerror() set. */
dl_object_t* dl_elf_map(const uint8_t* img, uint32_t fsize, const char* path);
void         dl_elf_unmap(dl_object_t* o);

/* Re-permission o's mapped pages to each PT_LOAD's own flags (W^X): text
   read-only-and-execute, data read-write, inter-segment padding no-access.
   Runs AFTER dl_relocate() (which writes into the still-writable data pages)
   and BEFORE the object's init. Returns 0 on success; a failed mprotect() on a
   window mapping fails the load rather than run code whose text is still
   writable. On the heap fallback mprotect() is a no-op and this returns 0. */
int          dl_elf_protect(dl_object_t* o);

/* Walk o->dyn_img into o->dyn, bounds-checking every offset against the
   mapped span. Returns 0 on success. */
int  dl_parse_dynamic(dl_object_t* o);
/* Reject a pointer derived from untrusted file data unless [p, p+n) lies
   entirely inside the object's mapped span. */
int  dl_span_ok(const dl_object_t* o, const void* p, uint64_t n);
/* Read a NUL-terminated string out of .dynstr at `off`. NULL when the offset
   is out of range or the table holds no terminator for it. */
const char* dl_dyn_string(const dl_object_t* o, uint64_t off);
int  dl_sym_count(dl_object_t* o);
int  dl_sym_at(dl_object_t* o, uint32_t idx, dl_sym_t* out);
const char* dl_sym_name(dl_object_t* o, const dl_sym_t* s);
/* SysV hash lookup. Returns the symbol index, or -1 when absent. */
int  dl_hash_lookup(dl_object_t* o, const char* name);
/* Linear scan over defined global/weak symbols; used by dladdr(). */
int  dl_sym_find_nearest(dl_object_t* o, uintptr_t vaddr, dl_sym_t* out);

uint32_t dl_reloc_count(dl_object_t* o, int table);
int      dl_reloc_at(dl_object_t* o, int table, uint32_t idx, dl_reloc_t* out);
/* Apply every relocation of every table of o. Returns 0 on success. */
int      dl_relocate(dl_object_t* o);

/* Publish freshly written bytes to the instruction fetch path. */
void     dl_cache_sync(dl_object_t* o);

/* --- dl_abi.c --- */
/* Look a name up in the runtime registry, then the static ABI tables.
   Caller already holds dl_lock. */
void* dl_host_abi_lookup(const char* name);

/* --- dl.c, exported to dl_abi.c --- */
/* Unlink o from the global object list and from every RTLD_GLOBAL slot.
   Does not touch refcount, deps or the mapping. */
void dl_object_unlink(dl_object_t* o);
/* Drop one reference. At zero: unlink, run fini, release deps, unmap. */
void dl_object_release(dl_object_t* o);

/* Upper bound on how many objects dl_build_scope() may return. */
#define DL_SEARCH_MAX    (DL_MAX_OBJECTS + 2)
/* Build the symbol-search order for `requester`. See the comment on the
   definition for the exact rules. Returns the number of entries written. */
uint32_t dl_build_scope(dl_object_t* requester, dl_object_t** out, uint32_t cap);

/* Symbol resolution across the global scope. `requester` may be NULL.
   On success stores the defining object in *defobj (may be NULL for the
   synthetic host object) and returns the final, biased address. */
uintptr_t dl_lookup_symbol(const char* name, dl_object_t* requester,
                           dl_object_t** defobj);

#ifdef __cplusplus
}
#endif

#endif /* DL_INTERNAL_H */
