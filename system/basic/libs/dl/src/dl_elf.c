/*
 * ELF parsing, mapping and relocation for the EwokOS dynamic loader.
 *
 * Everything in this file works on an in-memory file image handed over by
 * dl.c (read with vfs_readfile()), not on an fd: EwokOS has no page cache and
 * no file-backed mmap, so there is nothing to map lazily from. The image is
 * parsed, the PT_LOAD segments are copied into one contiguous anonymous
 * mapping, and the file buffer is then released.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <ewoksys/sys.h>

#include "dl_internal.h"

/* ------------------------------------------------------------------ */
/* host ELF identity                                                   */
/* ------------------------------------------------------------------ */

/*
 * A plugin has to be built for the machine the host is running on; there is no
 * emulation layer and no reason to pretend otherwise. The checks below produce
 * an error that names both sides, which beats a silent misparse.
 */
#if defined(__aarch64__)
#  define DL_EM_HOST    183            /* EM_AARCH64 */
#  define DL_CLS_HOST   ELFCLASS_64
#elif defined(__arm__)
#  define DL_EM_HOST    40             /* EM_ARM */
#  define DL_CLS_HOST   ELFCLASS_32
#elif defined(__x86_64__)
#  define DL_EM_HOST    62             /* EM_X86_64 */
#  define DL_CLS_HOST   ELFCLASS_64
#elif defined(__i386__)
#  define DL_EM_HOST    3              /* EM_386 */
#  define DL_CLS_HOST   ELFCLASS_32
#elif defined(__riscv)
#  define DL_EM_HOST    243            /* EM_RISCV */
#  if __riscv_xlen == 64
#    define DL_CLS_HOST ELFCLASS_64
#  else
#    define DL_CLS_HOST ELFCLASS_32
#  endif
#else
#  error "libdl: unknown host architecture"
#endif

/* ------------------------------------------------------------------ */
/* class-agnostic header views                                         */
/* ------------------------------------------------------------------ */

typedef struct {
	uint8_t  cls;
	uint16_t type;
	uint16_t machine;
	uint64_t entry;
	uint64_t phoff;
	uint16_t phentsize;
	uint16_t phnum;
} dl_ehdr_t;

typedef struct {
	uint32_t type;
	uint32_t flags;
	uint64_t off;
	uint64_t vaddr;
	uint64_t filesz;
	uint64_t memsz;
	uint64_t align;
} dl_phdr_t;

static int read_ehdr(const uint8_t* img, uint32_t fsize, dl_ehdr_t* h) {
	const elf_ident_t* id = (const elf_ident_t*)img;

	memset(h, 0, sizeof(*h));
	if(fsize < sizeof(elf_ident_t))
		return -1;
	if(id->magic != ELF_MAGIC)
		return -1;

	h->cls = id->cls;
	if(h->cls == ELFCLASS_64) {
		const elf64_header_t* e;
		if(fsize < sizeof(elf64_header_t))
			return -1;
		e = (const elf64_header_t*)img;
		h->type      = e->type;
		h->machine   = e->machine;
		h->entry     = e->entry;
		h->phoff     = e->phoff;
		h->phentsize = e->phentsize;
		h->phnum     = e->phnum;
	} else if(h->cls == ELFCLASS_32) {
		const elf_header_t* e;
		if(fsize < sizeof(elf_header_t))
			return -1;
		e = (const elf_header_t*)img;
		h->type      = e->type;
		h->machine   = e->machine;
		h->entry     = e->entry;
		h->phoff     = e->phoff;
		h->phentsize = e->phentsize;
		h->phnum     = e->phnum;
	} else {
		return -1;
	}
	return 0;
}

static int read_phdr(const uint8_t* img, uint32_t fsize, const dl_ehdr_t* h,
                     uint32_t idx, dl_phdr_t* p) {
	uint64_t at;

	memset(p, 0, sizeof(*p));
	/* the two real program-header layouts are 32 and 56 bytes; anything
	   smaller means phentsize is garbage and the arithmetic below would
	   walk off the end of the image */
	if(h->phentsize < 32)
		return -1;
	at = h->phoff + (uint64_t)idx * h->phentsize;
	if(at + h->phentsize < at)          /* wrapped */
		return -1;
	if(at + h->phentsize > fsize)
		return -1;

	if(h->cls == ELFCLASS_64) {
		const elf64_program_header_t* e =
			(const elf64_program_header_t*)(img + at);
		p->type   = e->type;
		p->flags  = e->flags;
		p->off    = e->off;
		p->vaddr  = e->vaddr;
		p->filesz = e->filesz;
		p->memsz  = e->memsz;
		p->align  = e->align;
	} else {
		const elf_program_header_t* e =
			(const elf_program_header_t*)(img + at);
		p->type   = e->type;
		p->flags  = e->flags;
		p->off    = e->off;
		p->vaddr  = e->vaddr;
		p->filesz = e->filesz;
		p->memsz  = e->memsz;
		p->align  = e->align;
	}
	return 0;
}

/* ------------------------------------------------------------------ */
/* span checking                                                       */
/* ------------------------------------------------------------------ */

int dl_span_ok(const dl_object_t* o, const void* p, uint64_t n) {
	uintptr_t lo, hi, v;

	if(o == NULL || o->base == NULL || o->span == 0)
		return 0;
	lo = (uintptr_t)o->base;
	hi = lo + o->span;
	v  = (uintptr_t)p;
	if(v < lo)
		return 0;
	if(v + n < v)                 /* wrapped */
		return 0;
	if(v + n > hi)
		return 0;
	return 1;
}

/* ------------------------------------------------------------------ */
/* mapping                                                             */
/* ------------------------------------------------------------------ */

/* Bounded copy that always terminates. libgloss's strncpy pads, which is not
   what any of the fixed-size name/path fields here want. */
void dl_strlcpy(char* dst, const char* src, uint32_t cap) {
	uint32_t i = 0;
	if(dst == NULL || cap == 0)
		return;
	if(src == NULL) {
		dst[0] = 0;
		return;
	}
	while(i + 1 < cap && src[i] != 0) {
		dst[i] = src[i];
		i++;
	}
	dst[i] = 0;
}

static const char* base_name_of(const char* path) {
	const char* p = path;
	const char* last = path;
	while(*p != 0) {
		if(*p == '/')
			last = p + 1;
		p++;
	}
	return last;
}

dl_object_t* dl_elf_map(const uint8_t* img, uint32_t fsize, const char* path) {
	dl_ehdr_t eh;
	dl_object_t* o;
	uint64_t lo = ~0ull, hi = 0;
	uint64_t dyn_off = 0, dyn_sz = 0;
	int have_load = 0, have_dyn = 0, have_tls = 0;
	uint32_t load_n = 0;
	uint32_t i;
	uint8_t* raw;
	size_t raw_n;

	if(img == NULL || fsize == 0 || path == NULL) {
		dl_set_error("dl_elf_map: no image");
		return NULL;
	}

	if(read_ehdr(img, fsize, &eh) != 0) {
		dl_set_error("%s: not a valid ELF image", path);
		return NULL;
	}
	if(eh.cls != DL_CLS_HOST) {
		dl_set_error("%s: ELF class %u, host is class %u",
		             path, (unsigned)eh.cls, (unsigned)DL_CLS_HOST);
		return NULL;
	}
	if(eh.machine != DL_EM_HOST) {
		dl_set_error("%s: e_machine %u, host is %u",
		             path, (unsigned)eh.machine, (unsigned)DL_EM_HOST);
		return NULL;
	}
	if(eh.type != ELFTYPE_SHARED) {
		dl_set_error("%s: e_type %u, expected ET_DYN(%u) - "
		             "a shared object must be linked with -shared",
		             path, (unsigned)eh.type, (unsigned)ELFTYPE_SHARED);
		return NULL;
	}

	/* --- first pass: work out the span and find PT_DYNAMIC --- */
	for(i = 0; i < eh.phnum; i++) {
		dl_phdr_t ph;
		if(read_phdr(img, fsize, &eh, i, &ph) != 0) {
			dl_set_error("%s: program header %u is truncated or malformed",
			             path, (unsigned)i);
			return NULL;
		}
		if(ph.type == PT_LOAD) {
			uint64_t align = ph.align ? ph.align : DL_BASE_ALIGN;
			uint64_t seglo = ph.vaddr & ~(align - 1);
			uint64_t seghi = ph.vaddr + ph.memsz;
			have_load = 1;
			load_n++;
			if(seghi < ph.vaddr) {         /* wrapped */
				dl_set_error("%s: PT_LOAD[%u] has an absurd memsz",
				             path, (unsigned)i);
				return NULL;
			}
			if(seglo < lo) lo = seglo;
			if(seghi > hi) hi = seghi;
			/* Fail here rather than letting memcpy read past the buffer:
			   a truncated image is the single most likely mistake when a
			   .so is copied onto the rootfs. */
			if(ph.filesz > 0 && (ph.off + ph.filesz > fsize ||
			                     ph.off + ph.filesz < ph.off)) {
				dl_set_error("%s: PT_LOAD[%u] wants %llu file bytes at "
				             "offset 0x%llx but the image is only %u bytes "
				             "(truncated copy?)",
				             path, (unsigned)i,
				             (unsigned long long)ph.filesz,
				             (unsigned long long)ph.off, (unsigned)fsize);
				return NULL;
			}
		} else if(ph.type == PT_DYNAMIC) {
			have_dyn = 1;
			dyn_off  = ph.off;
			dyn_sz   = ph.filesz;
		} else if(ph.type == PT_TLS) {
			have_tls = 1;
		}
	}

	if(!have_load) {
		dl_set_error("%s: no PT_LOAD segment", path);
		return NULL;
	}
	if(load_n > DL_MAX_LOAD_SEGS) {
		dl_set_error("%s: has %u PT_LOAD segments, more than the %d the "
		             "loader records for its W^X permission pass. A normal "
		             "-shared object has two; this one is not shaped like a "
		             "plugin",
		             path, (unsigned)load_n, DL_MAX_LOAD_SEGS);
		return NULL;
	}
	if(!have_dyn) {
		dl_set_error("%s: no PT_DYNAMIC segment - not a dynamically linked object",
		             path);
		return NULL;
	}
	if(have_tls) {
		dl_set_error("%s: uses thread-local storage (PT_TLS). EwokOS has no TLS "
		             "runtime, so rebuild the plugin without __thread",
		             path);
		return NULL;
	}
	if(dyn_sz == 0 || dyn_off + dyn_sz > fsize || dyn_off + dyn_sz < dyn_off) {
		dl_set_error("%s: PT_DYNAMIC is truncated", path);
		return NULL;
	}
	if(hi <= lo) {
		dl_set_error("%s: empty load span", path);
		return NULL;
	}

	o = (dl_object_t*)malloc(sizeof(dl_object_t));
	if(o == NULL) {
		dl_set_error("out of memory allocating a dl object for %s", path);
		return NULL;
	}
	memset(o, 0, sizeof(*o));
	dl_strlcpy(o->path, path, DL_PATH_LEN);
	o->is64    = (eh.cls == ELFCLASS_64) ? 1 : 0;
	o->load_lo = lo;
	o->span    = (uint32_t)(hi - lo);
	o->refcount= 0;

	/*
	 * libgloss's mmap() is heap backed and only guarantees 4k alignment,
	 * while a .so's p_align is 0x10000 on aarch64. Ask for one extra
	 * DL_BASE_ALIGN of slack and round the returned pointer up ourselves so
	 * the load bias satisfies the congruence rule for any page size the
	 * kernel may be running (machine.virt uses 16k). raw_alloc is the value
	 * munmap() must be handed back; base is the bias every unbiased VA is
	 * added to.
	 */
	raw_n = (size_t)o->span + DL_BASE_ALIGN;
	raw = (uint8_t*)mmap(NULL, raw_n, PROT_READ | PROT_WRITE | PROT_EXEC,
	                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if(raw == NULL || raw == (uint8_t*)MAP_FAILED) {
		dl_set_error("%s: could not map %u bytes for the load span",
		             path, (unsigned)raw_n);
		free(o);
		return NULL;
	}
	o->raw_alloc  = raw;
	o->alloc_span = (uint32_t)raw_n;
	o->base = (uint8_t*)(((uintptr_t)raw + (DL_BASE_ALIGN - 1)) &
	                     ~(uintptr_t)(DL_BASE_ALIGN - 1));

	/* --- second pass: copy the segments in --- */
	for(i = 0; i < eh.phnum; i++) {
		dl_phdr_t ph;
		uint8_t* dst;
		if(read_phdr(img, fsize, &eh, i, &ph) != 0)
			continue;              /* already validated above */
		if(ph.type != PT_LOAD)
			continue;
		dst = o->base + (ph.vaddr - lo);
		if(ph.filesz > 0)
			memcpy(dst, img + ph.off, (size_t)ph.filesz);
		/* .bss: memsz beyond filesz must read as zero. The window and the
		   heap fallback both hand back zero-filled pages, so this only has
		   to cover the tail of a segment whose filesz landed mid-page. */
		if(ph.memsz > ph.filesz)
			memset(dst + ph.filesz, 0, (size_t)(ph.memsz - ph.filesz));
		/* Record the segment for dl_elf_protect()'s W^X pass. The first pass
		   already rejected an object with more than DL_MAX_LOAD_SEGS, so this
		   cannot overflow; the guard is belt-and-braces. */
		if(o->segs_n < DL_MAX_LOAD_SEGS) {
			o->segs[o->segs_n].vbase = ph.vaddr - lo;
			o->segs[o->segs_n].memsz = ph.memsz;
			o->segs[o->segs_n].flags = ph.flags;
			o->segs_n++;
		}
	}

	/*
	 * PT_DYNAMIC is read from the file image, not from the mapped copy. The
	 * parse has to happen before we can trust any address in the copy, and
	 * every DT_* address value is unbiased anyway.
	 */
	o->dyn_img   = img + dyn_off;
	o->dyn_bytes = (uint32_t)dyn_sz;

	if(dl_parse_dynamic(o) != 0) {
		dl_elf_unmap(o);
		return NULL;
	}

	/* soname: DT_SONAME if present, else the file's base name */
	if(o->dyn.soname != 0) {
		const char* sn = dl_dyn_string(o, o->dyn.soname);
		if(sn != NULL)
			dl_strlcpy(o->soname, sn, DL_NAME_LEN);
	}
	if(o->soname[0] == 0)
		dl_strlcpy(o->soname, base_name_of(path), DL_NAME_LEN);

	return o;
}

void dl_elf_unmap(dl_object_t* o) {
	if(o == NULL)
		return;
	/* the synthetic host object owns no mapping */
	if(o->raw_alloc != NULL && !o->is_host)
		munmap(o->raw_alloc, o->alloc_span);
	free(o);
}

/*
 * W^X pass: re-permission the object's mapped pages to what each PT_LOAD asked
 * for. Runs after dl_relocate() - whose writes all land in the writable data
 * pages, never in text - and before the object's init.
 *
 * The whole span lives in one contiguous mapping, so per-segment permissions
 * are applied with mprotect() at the kernel's page granularity rather than by
 * mapping each segment separately. For every page in [base, base+span) the
 * permission is the union of the flags of the segments overlapping it; a page no
 * segment touches is inter-segment padding and becomes PROT_NONE, so a stray
 * pointer into the gap faults instead of silently reading zero fill.
 *
 * base is DL_BASE_ALIGN- (64k-) aligned and DL_BASE_ALIGN is a multiple of every
 * page size EwokOS builds for, so every run below is page-aligned and the kernel
 * accepts it. On the heap fallback mprotect() is a no-op returning success, so
 * this pass costs a few calls and changes nothing - W^X only bites when the
 * mapping came from the kernel window.
 */
int dl_elf_protect(dl_object_t* o) {
	uint32_t pg;
	uint64_t off;

	if(o == NULL || o->is_host || o->base == NULL)
		return 0;

	pg = (uint32_t)getpagesize();
	if(pg == 0 || (pg & (pg - 1)) != 0)
		pg = 4096;              /* not a power of two - do not guess */

	for(off = 0; off < o->span; ) {
		uint8_t* page = o->base + off;
		uint64_t run;
		int prot;
		uint32_t s;

		/* permission of this page: union of the segments overlapping it */
		prot = PROT_NONE;
		for(s = 0; s < o->segs_n; s++) {
			uint64_t slo = o->segs[s].vbase;
			uint64_t shi = slo + o->segs[s].memsz;
			if(slo < off + pg && off < shi) {
				if(o->segs[s].flags & PF_R) prot |= PROT_READ;
				if(o->segs[s].flags & PF_W) prot |= PROT_WRITE;
				if(o->segs[s].flags & PF_X) prot |= PROT_EXEC;
			}
		}

		/* grow the run across following pages that want the same permission,
		   so one mprotect() covers a whole segment instead of one per page */
		run = pg;
		while(off + run < o->span) {
			uint64_t noff = off + run;
			int np = PROT_NONE;
			for(s = 0; s < o->segs_n; s++) {
				uint64_t slo = o->segs[s].vbase;
				uint64_t shi = slo + o->segs[s].memsz;
				if(slo < noff + pg && noff < shi) {
					if(o->segs[s].flags & PF_R) np |= PROT_READ;
					if(o->segs[s].flags & PF_W) np |= PROT_WRITE;
					if(o->segs[s].flags & PF_X) np |= PROT_EXEC;
				}
			}
			if(np != prot)
				break;
			run += pg;
		}

		if(mprotect(page, (size_t)run, prot) != 0) {
			dl_set_error("%s: could not enforce W^X on the segment at "
			             "offset 0x%llx (mprotect of %llu bytes failed)",
			             o->path, (unsigned long long)off,
			             (unsigned long long)run);
			return -1;
		}
		off += run;
	}
	return 0;
}

/* ------------------------------------------------------------------ */
/* PT_DYNAMIC                                                          */
/* ------------------------------------------------------------------ */

/* Read a string out of the object's .dynstr. Returns NULL when the offset is
   out of range or the table holds no terminator for it. */
const char* dl_dyn_string(const dl_object_t* o, uint64_t off) {
	const uint8_t* st;

	if(o == NULL || o->dyn.strtab == 0 || o->dyn.strsz == 0)
		return NULL;
	if(off >= o->dyn.strsz)
		return NULL;
	st = o->base + o->dyn.strtab;
	if(!dl_span_ok(o, st, o->dyn.strsz))
		return NULL;
	if(memchr(st + off, 0, (size_t)(o->dyn.strsz - off)) == NULL)
		return NULL;
	return (const char*)(st + off);
}

/* Confirm a table described by (offset, size) really lies inside the mapping.
   Every DT_* value comes from an untrusted file, so each is checked once here
   rather than on every access. */
static int dyn_table_ok(const dl_object_t* o, uint64_t off, uint64_t size,
                        const char* what, const char* path) {
	if(size == 0)
		return 0;                 /* absent, which is fine */
	if(off == 0) {
		dl_set_error("%s: %s has a size of %llu bytes but no address",
		             path, what, (unsigned long long)size);
		return -1;
	}
	if(!dl_span_ok(o, o->base + off, size)) {
		dl_set_error("%s: %s at 0x%llx + %llu bytes falls outside the "
		             "%u-byte load span", path, what,
		             (unsigned long long)off, (unsigned long long)size,
		             (unsigned)o->span);
		return -1;
	}
	return 0;
}

int dl_parse_dynamic(dl_object_t* o) {
	uint64_t at = 0;
	uint32_t ent;
	uint32_t n;
	int saw_null = 0;

	memset(&o->dyn, 0, sizeof(o->dyn));
	if(o->dyn_img == NULL || o->dyn_bytes == 0) {
		dl_set_error("%s: empty PT_DYNAMIC", o->path);
		return -1;
	}

	ent = o->is64 ? (uint32_t)sizeof(Elf64_Dyn) : (uint32_t)sizeof(Elf32_Dyn);

	while(at + ent <= o->dyn_bytes) {
		uint64_t tag, val;
		if(o->is64) {
			const Elf64_Dyn* d = (const Elf64_Dyn*)(o->dyn_img + at);
			tag = d->d_tag;
			val = d->d_val;
		} else {
			const Elf32_Dyn* d = (const Elf32_Dyn*)(o->dyn_img + at);
			tag = d->d_tag;
			val = d->d_val;
		}
		at += ent;

		switch(tag) {
		case DT_NULL:          saw_null = 1; break;
		case DT_STRTAB:        o->dyn.strtab = val; break;
		case DT_STRSZ:         o->dyn.strsz = val; break;
		case DT_SYMTAB:        o->dyn.symtab = val; break;
		case DT_SYMENT:        o->dyn.syment = val; break;
		case DT_HASH:          o->dyn.hash = val; break;
		case DT_GNU_HASH:      o->dyn.gnu_hash = 1; break;
		case DT_RELA:          o->dyn.rela = val; break;
		case DT_RELASZ:        o->dyn.relasz = val; break;
		case DT_RELAENT:       o->dyn.relaent = val; break;
		case DT_REL:           o->dyn.rel = val; break;
		case DT_RELSZ:         o->dyn.relsz = val; break;
		case DT_RELENT:        o->dyn.relent = val; break;
		case DT_JMPREL:        o->dyn.jmprel = val; break;
		case DT_PLTRELSZ:      o->dyn.pltrelsz = val; break;
		case DT_PLTREL:        o->dyn.pltrel = val; break;
		case DT_INIT:          o->dyn.init = val; break;
		case DT_FINI:          o->dyn.fini = val; break;
		case DT_INIT_ARRAY:    o->dyn.init_array = val; break;
		case DT_INIT_ARRAYSZ:  o->dyn.init_arraysz = val; break;
		case DT_FINI_ARRAY:    o->dyn.fini_array = val; break;
		case DT_FINI_ARRAYSZ:  o->dyn.fini_arraysz = val; break;
		case DT_SONAME:        o->dyn.soname = val; break;
		case DT_SYMBOLIC:      o->dyn.symbolic = 1; break;
		case DT_BIND_NOW:      o->dyn.bind_now = 1; break;
		case DT_TEXTREL:       o->dyn.textrel = 1; break;
		case DT_NEEDED:
			if(o->dyn.needed_n < DL_MAX_NEEDED)
				o->dyn.needed[o->dyn.needed_n++] = val;
			else
				dl_set_error("%s: more than %d DT_NEEDED entries; "
				             "raise DL_MAX_NEEDED", o->path, DL_MAX_NEEDED);
			break;
		default:
			/* Unknown tags are ignorable by definition in the ELF ABI -
			   DT_DEBUG, DT_VERSYM, DT_RPATH, DT_RUNPATH, DT_PLTGOT and
			   the processor-specific range all land here. */
			break;
		}
		if(saw_null)
			break;
	}

	if(!saw_null) {
		dl_set_error("%s: PT_DYNAMIC has no DT_NULL terminator within %u bytes",
		             o->path, (unsigned)o->dyn_bytes);
		return -1;
	}
	if(o->dyn.strtab == 0 || o->dyn.strsz == 0) {
		dl_set_error("%s: no DT_STRTAB/DT_STRSZ", o->path);
		return -1;
	}
	if(o->dyn.symtab == 0 || o->dyn.syment == 0) {
		dl_set_error("%s: no DT_SYMTAB/DT_SYMENT", o->path);
		return -1;
	}
	/*
	 * No DT_* tag reports how long .dynsym is. The SysV hash table's nchain
	 * field is by definition the number of symbol-table entries, which is
	 * why every loader derives the length from it. Without DT_HASH there is
	 * no way to bound a symbol walk, so a GNU-hash-only object is refused
	 * rather than guessed at.
	 */
	if(o->dyn.hash == 0) {
		if(o->dyn.gnu_hash)
			dl_set_error("%s: carries only .gnu.hash; libdl implements the "
			             "SysV DT_HASH table. Link without -Wl,--hash-style=gnu",
			             o->path);
		else
			dl_set_error("%s: no DT_HASH, so the symbol table length is "
			             "unknowable", o->path);
		return -1;
	}

	if(dyn_table_ok(o, o->dyn.strtab, o->dyn.strsz, ".dynstr", o->path) < 0)
		return -1;

	n = (uint32_t)dl_sym_count(o);
	if(n == 0) {
		dl_set_error("%s: DT_HASH reports an empty symbol table", o->path);
		return -1;
	}
	if(dyn_table_ok(o, o->dyn.symtab,
	                (uint64_t)n * o->dyn.syment, ".dynsym", o->path) < 0)
		return -1;
	if(dyn_table_ok(o, o->dyn.rela, o->dyn.relasz, ".rela.dyn", o->path) < 0)
		return -1;
	if(dyn_table_ok(o, o->dyn.rel, o->dyn.relsz, ".rel.dyn", o->path) < 0)
		return -1;
	if(dyn_table_ok(o, o->dyn.jmprel, o->dyn.pltrelsz, ".rel[a].plt", o->path) < 0)
		return -1;
	if(dyn_table_ok(o, o->dyn.init_array, o->dyn.init_arraysz,
	                ".init_array", o->path) < 0)
		return -1;
	if(dyn_table_ok(o, o->dyn.fini_array, o->dyn.fini_arraysz,
	                ".fini_array", o->path) < 0)
		return -1;

	if(o->dyn.textrel) {
		/*
		 * DT_TEXTREL means the object has relocations that write into a
		 * segment the linker marked read-only/executable. That is
		 * irreconcilable with W^X: dl_elf_protect() is about to make the text
		 * segment read-only-and-executable, and a text relocation would either
		 * have needed it left writable-and-executable or would now fault. Such
		 * an object was not built with -fPIC; refuse it rather than quietly
		 * weaken the guarantee the dedicated window exists to provide.
		 */
		dl_set_error("%s: has DT_TEXTREL (text relocations). Rebuild the "
		             "plugin with -fPIC; EwokOS enforces W^X on loaded objects "
		             "and cannot apply relocations to an executable segment",
		             o->path);
		return -1;
	}
	return 0;
}

/* ------------------------------------------------------------------ */
/* symbols                                                             */
/* ------------------------------------------------------------------ */

int dl_sym_count(dl_object_t* o) {
	const Elf_Hash_Header* h;

	if(o == NULL || o->dyn.hash == 0 || o->base == NULL)
		return 0;
	h = (const Elf_Hash_Header*)(o->base + o->dyn.hash);
	if(!dl_span_ok(o, h, sizeof(*h)))
		return 0;
	return (int)h->nchain;
}

int dl_sym_at(dl_object_t* o, uint32_t idx, dl_sym_t* out) {
	const uint8_t* st;
	uint64_t ent, at;

	memset(out, 0, sizeof(*out));
	if(o == NULL || o->dyn.symtab == 0 || o->dyn.syment == 0)
		return -1;
	ent = o->dyn.syment;
	if((uint64_t)idx >= (uint64_t)dl_sym_count(o))
		return -1;
	st = o->base + o->dyn.symtab;
	at = (uint64_t)idx * ent;
	if(!dl_span_ok(o, st + at, ent))
		return -1;

	if(o->is64) {
		const Elf64_Sym* s = (const Elf64_Sym*)(st + at);
		out->value    = s->st_value;
		out->size     = s->st_size;
		out->name_off = (uint32_t)s->st_name;
		out->shndx    = s->st_shndx;
		out->bind     = (uint8_t)ELF64_ST_BIND(s->st_info);
		out->type     = (uint8_t)ELF64_ST_TYPE(s->st_info);
	} else {
		const Elf32_Sym* s = (const Elf32_Sym*)(st + at);
		out->value    = s->st_value;
		out->size     = s->st_size;
		out->name_off = s->st_name;
		out->shndx    = s->st_shndx;
		out->bind     = (uint8_t)ELF32_ST_BIND(s->st_info);
		out->type     = (uint8_t)ELF32_ST_TYPE(s->st_info);
	}
	return 0;
}

const char* dl_sym_name(dl_object_t* o, const dl_sym_t* s) {
	if(o == NULL || s == NULL)
		return NULL;
	return dl_dyn_string(o, s->name_off);
}

static uint32_t dl_elf_hash(const char* name) {
	uint32_t h = 0, g;
	while(*name != 0) {
		h = (h << 4) + (uint8_t)*name++;
		g = h & 0xf0000000u;
		if(g != 0)
			h ^= g >> 24;
		h &= ~g;
	}
	return h;
}

int dl_hash_lookup(dl_object_t* o, const char* name) {
	const Elf_Hash_Header* h;
	const uint32_t* bucket;
	const uint32_t* chain;
	uint32_t i, guard;

	if(o == NULL || name == NULL || o->dyn.hash == 0)
		return -1;
	h = (const Elf_Hash_Header*)(o->base + o->dyn.hash);
	if(!dl_span_ok(o, h, sizeof(*h)))
		return -1;
	if(h->nbucket == 0 || h->nchain == 0)
		return -1;
	if(!dl_span_ok(o, h, sizeof(*h) +
	                   (uint64_t)(h->nbucket + h->nchain) * 4))
		return -1;

	bucket = (const uint32_t*)(h + 1);
	chain  = bucket + h->nbucket;

	i = bucket[dl_elf_hash(name) % h->nbucket];
	/* nchain bounds the length of any well-formed chain; counting against it
	   means a corrupt table cannot spin forever */
	for(guard = 0; i != 0 && i < h->nchain && guard < h->nchain; guard++) {
		dl_sym_t s;
		if(dl_sym_at(o, i, &s) != 0)
			return -1;
		/*
		 * Skip SHN_UNDEF deliberately: a .so's own undefined placeholder for
		 * a symbol it imports must not satisfy its own reference, or every
		 * external call would resolve to address 0.
		 */
		if(s.shndx != SHN_UNDEF) {
			const char* sn = dl_sym_name(o, &s);
			if(sn != NULL && strcmp(sn, name) == 0)
				return (int)i;
		}
		i = chain[i];
	}
	return -1;
}

int dl_sym_find_nearest(dl_object_t* o, uintptr_t vaddr, dl_sym_t* out) {
	int n, i, best = -1;
	uint64_t off, best_delta = ~0ull;
	dl_sym_t bs;

	if(o == NULL || o->base == NULL || o->span == 0)
		return -1;
	if(vaddr < (uintptr_t)o->base)
		return -1;
	off = vaddr - (uintptr_t)o->base;
	if(off >= o->span)
		return -1;

	memset(&bs, 0, sizeof(bs));
	n = dl_sym_count(o);
	for(i = 0; i < n; i++) {
		dl_sym_t s;
		uint64_t delta;
		if(dl_sym_at(o, (uint32_t)i, &s) != 0)
			continue;
		if(s.shndx == SHN_UNDEF)
			continue;
		if(s.type != STT_FUNC && s.type != STT_OBJECT && s.type != STT_NOTYPE)
			continue;
		if(s.value > off)
			continue;
		delta = off - s.value;
		/* a nonzero size means the symbol really owns [value, value+size);
		   an address past that belongs to whatever follows, which is often
		   unnamed, so keep it only as a fallback */
		if(s.size != 0 && delta >= s.size)
			continue;
		if(delta < best_delta) {
			best_delta = delta;
			best = i;
			bs = s;
			if(delta == 0)
				break;
		}
	}
	if(best < 0)
		return -1;
	*out = bs;
	return best;
}

/* ------------------------------------------------------------------ */
/* relocations                                                         */
/* ------------------------------------------------------------------ */

/* Default entry sizes, used when the DT_*ENT tag is missing. RELA is
   12/24 bytes and REL is 8/16 bytes for ELFCLASS32/64 respectively. */
static uint64_t reloc_ent(const dl_object_t* o, int table, int* rela) {
	switch(table) {
	case DL_RT_RELA:
		*rela = 1;
		return o->dyn.relaent ? o->dyn.relaent
		                      : (o->is64 ? 24u : 12u);
	case DL_RT_REL:
		*rela = 0;
		return o->dyn.relent ? o->dyn.relent
		                     : (o->is64 ? 16u : 8u);
	case DL_RT_PLT:
		*rela = (o->dyn.pltrel == DT_RELA) ? 1 : 0;
		if(*rela)
			return o->is64 ? 24u : 12u;
		return o->is64 ? 16u : 8u;
	default:
		*rela = 0;
		return 0;
	}
}

static uint64_t reloc_off(const dl_object_t* o, int table) {
	switch(table) {
	case DL_RT_RELA: return o->dyn.rela;
	case DL_RT_REL:  return o->dyn.rel;
	case DL_RT_PLT:  return o->dyn.jmprel;
	default:         return 0;
	}
}

static uint64_t reloc_sz(const dl_object_t* o, int table) {
	switch(table) {
	case DL_RT_RELA: return o->dyn.relasz;
	case DL_RT_REL:  return o->dyn.relsz;
	case DL_RT_PLT:  return o->dyn.pltrelsz;
	default:         return 0;
	}
}

uint32_t dl_reloc_count(dl_object_t* o, int table) {
	uint64_t sz, ent;
	int rela;

	if(o == NULL)
		return 0;
	ent = reloc_ent(o, table, &rela);
	sz  = reloc_sz(o, table);
	if(ent == 0 || sz == 0)
		return 0;
	return (uint32_t)(sz / ent);
}

int dl_reloc_at(dl_object_t* o, int table, uint32_t idx, dl_reloc_t* out) {
	uint64_t ent, at, toff;
	const uint8_t* p;
	int rela;

	memset(out, 0, sizeof(*out));
	if(o == NULL)
		return -1;
	ent  = reloc_ent(o, table, &rela);
	toff = reloc_off(o, table);
	if(ent == 0 || toff == 0)
		return -1;
	if((uint64_t)idx >= (uint64_t)dl_reloc_count(o, table))
		return -1;
	at = toff + (uint64_t)idx * ent;
	p  = o->base + at;
	if(!dl_span_ok(o, p, ent))
		return -1;

	if(o->is64) {
		if(rela) {
			const Elf64_Rela* r = (const Elf64_Rela*)p;
			out->offset = r->r_offset;
			out->symidx = (uint32_t)ELF64_R_SYM(r->r_info);
			out->type   = (uint32_t)ELF64_R_TYPE(r->r_info);
			out->addend = (uint64_t)r->r_addend;
		} else {
			const Elf64_Rel* r = (const Elf64_Rel*)p;
			out->offset = r->r_offset;
			out->symidx = (uint32_t)ELF64_R_SYM(r->r_info);
			out->type   = (uint32_t)ELF64_R_TYPE(r->r_info);
			out->addend = 0;
		}
	} else {
		if(rela) {
			const Elf32_Rela* r = (const Elf32_Rela*)p;
			out->offset = r->r_offset;
			out->symidx = ELF32_R_SYM(r->r_info);
			out->type   = ELF32_R_TYPE(r->r_info);
			out->addend = (uint64_t)(int64_t)r->r_addend;
		} else {
			const Elf32_Rel* r = (const Elf32_Rel*)p;
			out->offset = r->r_offset;
			out->symidx = ELF32_R_SYM(r->r_info);
			out->type   = ELF32_R_TYPE(r->r_info);
			out->addend = 0;
		}
	}
	out->has_addend = rela;
	return 0;
}

static int reloc_oob(dl_object_t* o, const dl_reloc_t* r, uint32_t need) {
	dl_set_error("%s: relocation type %u at offset 0x%llx needs %u bytes, "
	             "which is outside the %u-byte load span",
	             o->path, (unsigned)r->type,
	             (unsigned long long)r->offset, (unsigned)need,
	             (unsigned)o->span);
	return -1;
}

static int reloc_unsupported(dl_object_t* o, uint32_t type, const char* why) {
	dl_set_error("%s: unsupported relocation type %u (%s)",
	             o->path, (unsigned)type, why);
	return -1;
}

/*
 * Apply one relocation.
 *
 * `symval` is already the final biased address of the referenced symbol (0 for
 * an unresolvable weak symbol, 0 for a RELATIVE entry where symidx is 0).
 * `addend` was filled in by dl_reloc_at() for RELA; for REL it is read from
 * the in-place word here, which is the only way a REL addend exists.
 */
static int dl_apply_reloc(dl_object_t* o, const dl_reloc_t* r, uint64_t symval) {
	uint8_t* where;
	uint64_t addend = r->addend;
	uint64_t P, S;

	if(!dl_span_ok(o, o->base + r->offset, 1))
		return reloc_oob(o, r, 1);
	where = o->base + r->offset;

	if(!r->has_addend) {
		if(!dl_span_ok(o, where, o->is64 ? 8u : 4u))
			return reloc_oob(o, r, o->is64 ? 8u : 4u);
		addend = o->is64 ? *(uint64_t*)where : (uint64_t)*(uint32_t*)where;
	}

	S = symval;
	P = (uint64_t)(uintptr_t)where;

#if defined(__aarch64__)
	switch(r->type) {
	case R_AARCH64_NONE:
		return 0;
	case R_AARCH64_RELATIVE:
		if(!dl_span_ok(o, where, 8)) return reloc_oob(o, r, 8);
		*(uint64_t*)where = (uint64_t)(uintptr_t)o->base + addend;
		return 0;
	case R_AARCH64_GLOB_DAT:
	case R_AARCH64_JUMP_SLOT:
	case R_AARCH64_ABS64:
		if(!dl_span_ok(o, where, 8)) return reloc_oob(o, r, 8);
		*(uint64_t*)where = S + addend;
		return 0;
	case R_AARCH64_ABS32:
		if(!dl_span_ok(o, where, 4)) return reloc_oob(o, r, 4);
		*(uint32_t*)where = (uint32_t)(S + addend);
		return 0;
	case R_AARCH64_PREL64:
		if(!dl_span_ok(o, where, 8)) return reloc_oob(o, r, 8);
		*(int64_t*)where = (int64_t)(S + addend) - (int64_t)P;
		return 0;
	case R_AARCH64_PREL32:
		if(!dl_span_ok(o, where, 4)) return reloc_oob(o, r, 4);
		*(int32_t*)where = (int32_t)((int64_t)(S + addend) - (int64_t)P);
		return 0;
	case R_AARCH64_COPY:
		return reloc_unsupported(o, r->type,
			"R_AARCH64_COPY interposes a host global; EwokOS has no "
			"copy-relocation support - mark the symbol hidden in the plugin");
	case R_AARCH64_CALL26:
	case R_AARCH64_JUMP26:
		return reloc_unsupported(o, r->type,
			"a non-PIC +/-128MB branch, which cannot reach a load base "
			"chosen at runtime - rebuild the plugin with -fPIC");
	case R_AARCH64_TLS_DTPMOD64:
	case R_AARCH64_TLS_DTPREL64:
	case R_AARCH64_TLS_TPREL64:
	case R_AARCH64_TLSDESC:
		return reloc_unsupported(o, r->type,
			"thread-local storage, which EwokOS does not provide");
	default:
		return reloc_unsupported(o, r->type, "unknown to libdl");
	}

#elif defined(__arm__)
	switch(r->type) {
	case R_ARM_NONE:
		return 0;
	case R_ARM_RELATIVE:
		if(!dl_span_ok(o, where, 4)) return reloc_oob(o, r, 4);
		*(uint32_t*)where = (uint32_t)((uintptr_t)o->base + addend);
		return 0;
	case R_ARM_GLOB_DAT:
	case R_ARM_JUMP_SLOT:
	case R_ARM_ABS32:
	case R_ARM_TARGET1:      /* TARGET1 is ABS32 for -shared output */
		if(!dl_span_ok(o, where, 4)) return reloc_oob(o, r, 4);
		*(uint32_t*)where = (uint32_t)(S + addend);
		return 0;
	case R_ARM_PREL31:
		/* .ARM.extab entries: 31-bit PC-relative, top bit is preserved */
		if(!dl_span_ok(o, where, 4)) return reloc_oob(o, r, 4);
		*(uint32_t*)where = (*(uint32_t*)where & 0x80000000u) |
			((uint32_t)((int64_t)(S + addend) - (int64_t)P) & 0x7fffffffu);
		return 0;
	case R_ARM_COPY:
		return reloc_unsupported(o, r->type, "copy relocation");
	case R_ARM_PC24:
	case R_ARM_CALL28:
	case R_ARM_JUMP24:
		return reloc_unsupported(o, r->type,
			"a non-PIC +/-32MB branch - rebuild the plugin with -fPIC");
	default:
		return reloc_unsupported(o, r->type, "unknown to libdl");
	}

#elif defined(__x86_64__)
	switch(r->type) {
	case R_X86_64_NONE:
		return 0;
	case R_X86_64_RELATIVE:
		if(!dl_span_ok(o, where, 8)) return reloc_oob(o, r, 8);
		*(uint64_t*)where = (uint64_t)(uintptr_t)o->base + addend;
		return 0;
	case R_X86_64_GLOB_DAT:
	case R_X86_64_JUMP_SLOT:
	case R_X86_64_64:
		if(!dl_span_ok(o, where, 8)) return reloc_oob(o, r, 8);
		*(uint64_t*)where = S + addend;
		return 0;
	case R_X86_64_PC32:
		if(!dl_span_ok(o, where, 4)) return reloc_oob(o, r, 4);
		*(int32_t*)where = (int32_t)((int64_t)(S + addend) - (int64_t)P);
		return 0;
	case R_X86_64_COPY:
		return reloc_unsupported(o, r->type, "copy relocation");
	default:
		return reloc_unsupported(o, r->type, "unknown to libdl");
	}

#elif defined(__i386__)
	switch(r->type) {
	case R_386_NONE:
		return 0;
	case R_386_RELATIVE:
		if(!dl_span_ok(o, where, 4)) return reloc_oob(o, r, 4);
		*(uint32_t*)where = (uint32_t)((uintptr_t)o->base + addend);
		return 0;
	case R_386_GLOB_DAT:
	case R_386_JMP_SLOT:
	case R_386_32:
		if(!dl_span_ok(o, where, 4)) return reloc_oob(o, r, 4);
		*(uint32_t*)where = (uint32_t)(S + addend);
		return 0;
	case R_386_PC32:
		if(!dl_span_ok(o, where, 4)) return reloc_oob(o, r, 4);
		*(int32_t*)where = (int32_t)((int64_t)(S + addend) - (int64_t)P);
		return 0;
	case R_386_COPY:
		return reloc_unsupported(o, r->type, "copy relocation");
	default:
		return reloc_unsupported(o, r->type, "unknown to libdl");
	}

#elif defined(__riscv)
	switch(r->type) {
	case R_RISCV_NONE:
		return 0;
	case R_RISCV_RELATIVE:
#  if __riscv_xlen == 64
		if(!dl_span_ok(o, where, 8)) return reloc_oob(o, r, 8);
		*(uint64_t*)where = (uint64_t)(uintptr_t)o->base + addend;
#  else
		if(!dl_span_ok(o, where, 4)) return reloc_oob(o, r, 4);
		*(uint32_t*)where = (uint32_t)((uintptr_t)o->base + addend);
#  endif
		return 0;
	case R_RISCV_JUMP_SLOT:
#  if __riscv_xlen == 64
		if(!dl_span_ok(o, where, 8)) return reloc_oob(o, r, 8);
		*(uint64_t*)where = S + addend;
#  else
		if(!dl_span_ok(o, where, 4)) return reloc_oob(o, r, 4);
		*(uint32_t*)where = (uint32_t)(S + addend);
#  endif
		return 0;
	case R_RISCV_64:
#  if __riscv_xlen == 64
		if(!dl_span_ok(o, where, 8)) return reloc_oob(o, r, 8);
		*(uint64_t*)where = S + addend;
		return 0;
#  else
		return reloc_unsupported(o, r->type, "R_RISCV_64 on a 32-bit host");
#  endif
	case R_RISCV_32:
		if(!dl_span_ok(o, where, 4)) return reloc_oob(o, r, 4);
		*(uint32_t*)where = (uint32_t)(S + addend);
		return 0;
	case R_RISCV_COPY:
		return reloc_unsupported(o, r->type, "copy relocation");
	default:
		return reloc_unsupported(o, r->type, "unknown to libdl");
	}

#else
	(void)S; (void)P; (void)addend; (void)where;
	return reloc_unsupported(o, r->type, "libdl was built for an unknown host");
#endif
}

int dl_relocate(dl_object_t* o) {
	static const int tables[3] = { DL_RT_RELA, DL_RT_REL, DL_RT_PLT };
	int t;

	if(o == NULL)
		return -1;
	if(o->relocated)
		return 0;

	for(t = 0; t < 3; t++) {
		int table = tables[t];
		uint32_t n = dl_reloc_count(o, table);
		uint32_t i;

		for(i = 0; i < n; i++) {
			dl_reloc_t r;
			uint64_t symval = 0;

			if(dl_reloc_at(o, table, i, &r) != 0) {
				dl_set_error("%s: relocation %u of table %d is malformed",
				             o->path, (unsigned)i, table);
				return -1;
			}

			/* symidx 0 is SHN_UNDEF: RELATIVE and friends carry their
			   value in the addend and reference nothing */
			if(r.symidx != 0) {
				dl_sym_t s;
				const char* symname;
				if(dl_sym_at(o, r.symidx, &s) != 0) {
					dl_set_error("%s: relocation %u references symbol index "
					             "%u, which is out of range",
					             o->path, (unsigned)i, (unsigned)r.symidx);
					return -1;
				}
				symname = dl_sym_name(o, &s);
				if(symname == NULL)
					symname = "<unnamed>";

				symval = (uint64_t)dl_lookup_symbol(symname, o, NULL);
				if(symval == 0) {
					if(s.bind == STB_WEAK) {
						/*
						 * An undefined weak symbol binds to 0 rather than
						 * failing the load; that is the whole point of
						 * STB_WEAK and plugins use it for optional host
						 * hooks.
						 */
						symval = 0;
					} else {
						dl_set_error("%s: undefined symbol '%s' "
						             "(the host publishes its ABI through "
						             "<dl/dl.h>; see dl_host_publish)",
						             o->path, symname);
						return -1;
					}
				}
			}

			if(dl_apply_reloc(o, &r, symval) != 0)
				return -1;
		}
	}

	o->relocated = 1;
	return 0;
}

/* ------------------------------------------------------------------ */
/* cache maintenance                                                   */
/* ------------------------------------------------------------------ */

void dl_cache_sync(dl_object_t* o) {
	if(o == NULL || o->is_host || o->base == NULL || o->span == 0)
		return;
	/*
	 * The code bytes got here with memcpy, i.e. as ordinary data stores. On
	 * AArch64 instruction fetch is not coherent with the D-cache, and EL0
	 * may not run DC CIVAC / IC IVAU itself because SCTLR_EL1.UCI is left
	 * clear, so SYS_CACHE_SYNC is the only route to the PoU. On the other
	 * supported architectures the syscall is a cheap no-op.
	 */
	sys_cache_sync((void*)o->base, o->span);
}
