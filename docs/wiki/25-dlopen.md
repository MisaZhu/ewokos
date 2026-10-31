# 25 Runtime Dynamic Loading: How `dlopen()` Works on a Static OS

> Language: **English** | [中文](25-dlopen.zh.md)
>
> Goal: understand *why* an operating system that links every program
> statically can still offer `dlopen()`; see the three pieces that make it
> work — a kernel anonymous-mapping window with real W^X permissions, a
> cache-sync syscall, and a user-space ELF dynamic loader (`libdl`) plus a
> build-time generated host ABI table; and follow one plugin from `dlopen()`
> to a running function and back through `dlclose()`.
>
> This chapter builds on [05 Memory & MMU](05-mmu.md), [07 Processes](07-process.md),
> [08 System Calls](08-syscall.md) and [23 Cache Maintenance](23-cache.md), and
> sits in the library series beside [16 Library Overview](16-libs-overview.md)
> and [17 The libc](17-libc.md). Terminology is collected in the [glossary](99-glossary.md).

Every other chapter assumed a program is one static ELF image: the kernel maps
it, jumps to `_start`, and everything the program calls is already linked in.
That model has no room for a *plugin* — a `.so` file discovered and loaded at
run time. This chapter removes that limitation and shows exactly how EwokOS
adds runtime dynamic loading without giving up its static-link design.

The short version: EwokOS does **not** grow an `ld.so`. Instead `dlopen()` is a
plain library (`libdl`) that does, in user space, the subset of a dynamic
linker's job that a plugin actually needs, on top of two new kernel primitives.
It is implemented and boot-verified on **AArch64** (`machine.virt`); the
self-test `bin/dltest` reports **94 checks, 0 failures** under QEMU.

Every code reference below points at a real file:

| Layer | File |
|---|---|
| POSIX API header | [`system/basic/libs/dl/include/dlfcn.h`](../../system/basic/libs/dl/include/dlfcn.h) |
| EwokOS host-ABI extensions | [`system/basic/libs/dl/include/dl/dl.h`](../../system/basic/libs/dl/include/dl/dl.h) |
| Loader core (objects, scope, lifecycle) | [`system/basic/libs/dl/src/dl.c`](../../system/basic/libs/dl/src/dl.c) |
| ELF/dynamic parser + relocation + W^X pass | [`system/basic/libs/dl/src/dl_elf.c`](../../system/basic/libs/dl/src/dl_elf.c) |
| Host ABI lookup (binary search + registry) | [`system/basic/libs/dl/src/dl_abi.c`](../../system/basic/libs/dl/src/dl_abi.c) |
| Export-table generator | [`system/basic/libs/dl/gen_export.sh`](../../system/basic/libs/dl/gen_export.sh) |
| ELF dynamic structs | [`system/basic/libs/elf/include/elf/elf_dyn.h`](../../system/basic/libs/elf/include/elf/elf_dyn.h) |
| Syscall numbers & ABI constants | [`kernel/kernel/include/syscalls.h`](../../kernel/kernel/include/syscalls.h) |
| Syscall handlers | [`kernel/kernel/src/svc.c`](../../kernel/kernel/src/svc.c) |
| Anonymous window + W^X mapping | [`kernel/kernel/src/mm/mmu.c`](../../kernel/kernel/src/mm/mmu.c), [`kernel/kernel/include/mm/mmu.h`](../../kernel/kernel/include/mm/mmu.h) |
| libc `mmap`/`mprotect`/`getpagesize` routing | [`system/basic/libc/libgloss/compat.c`](../../system/basic/libc/libgloss/compat.c) |
| The self-test | [`system/basic/bin/dltest/`](../../system/basic/bin/dltest/) |

---

## 25.1 Why a static OS has no `ld.so`

On Linux a plugin's undefined symbols (`printf`, and the main program's own
functions) resolve against the executable because the executable carries a
`.dynsym`, populated by linking with `-Wl,-E` / `--export-dynamic`, and a
dynamic loader (`ld.so`) reads that table at run time.

EwokOS programs are fully static `ET_EXEC` links produced by
`aarch64-none-elf-ld` directly. For an `ET_EXEC` link GNU ld creates **no
dynamic section at all**, so `--export-dynamic` is a silent no-op: there is
nothing for a loader to read, and no `ld.so` exists to read it. Two
consequences shape the whole design:

1. The host must **publish its ABI explicitly** — it cannot rely on a `.dynsym`
   it does not have (§25.5).
2. `dlopen()` must be **an ordinary library**, not a special interpreter loaded
   before `main`. It runs inside the already-started process, using the same
   libc and syscalls everything else uses.

So EwokOS keeps its static-link simplicity and adds dynamic loading as a
*plugin facility*: `dlopen()` loads an `ET_DYN` object built with
`-fPIC -shared`, relocates it against the host's exported ABI, and hands back
an opaque handle.

---

## 25.2 The three-piece design

```
   host app (static ET_EXEC)                plugin (ET_DYN .so, -fPIC)
   ┌─────────────────────────┐             ┌──────────────────────────┐
   │ dlopen("/lib/p.so",…)   │             │  .text  (R+X)            │
   │   │                     │             │  .data  (RW)             │
   │   ▼                     │             │  .init_array/.fini_array │
   │ libdl  ────────────────┼── loads ───▶│  .dynsym/.hash/.rela.*   │
   │   │  host ABI table     │             └──────────────────────────┘
   │   │  (gen_export.sh)    │                      ▲        ▲
   │   ▼                     │                      │        │
   │ mmap/mprotect/cache ────┼── syscalls ──────────┘        │
   └─────────────────────────┘                                │
                    │                                          │
                    ▼                                          │
   kernel: SYS_MMAP_ANON / SYS_MPROTECT / SYS_MUNMAP           │
           SYS_CACHE_SYNC  ───────────────────────────────────┘
           DL_WINDOW_* per-process VA window (W^X)
```

- **Kernel** provides two things user space cannot do itself: make freshly
  written instruction bytes executable (`SYS_CACHE_SYNC`), and hand out
  anonymous memory with *per-page permissions* (`SYS_MMAP_ANON` +
  `SYS_MPROTECT` over the `DL_WINDOW_*` reservation).
- **libdl** does the ELF work in user space: parse, map, relocate, run
  constructors, manage the object graph and reference counts.
- **Host ABI table** replaces the missing `.dynsym` so a plugin's undefined
  symbols have something to bind to.

---

## 25.3 Kernel piece 1 — `SYS_CACHE_SYNC`: making new code executable

AArch64 splits the caches: bytes written through the D-cache are **not** visible
to the instruction fetcher until they are cleaned to the *Point of Unification*
(PoU) and the I-cache is invalidated (see [23 Cache Maintenance](23-cache.md)).
A loader that `memcpy`s a plugin's `.text` into a mapping therefore must publish
those bytes before calling into them.

EL0 cannot do this itself: `SCTLR_EL1.UCI` is left clear, so a user-space
`dc civac` / `ic ivau` traps. `SYS_CACHE_SYNC(vaddr, size)` is the only route:

```c
/* kernel/kernel/src/svc.c */
static int32_t sys_cache_sync(ewokos_addr_t vaddr, uint32_t size) {
    proc_t* cproc = get_current_proc();
    if(cproc == NULL || cproc->space == NULL) return -1;
    if(size == 0) return 0;
    if(!user_ptr_ok(cproc, vaddr, size)) return -1;   /* never steer it over kernel VA */
    /* walk the range page by page, skipping holes, cleaning through the kernel alias */
    …
    invalidate_icache_all();                          /* broadcast (ICIALLUIS) */
    return 0;
}
```

libgloss exposes it as `__clear_cache()`, which libdl calls after copying each
object. In the boot log this is the reason the line `A: ctor ran (.init_array)`
can appear at all — the plugin's freshly-copied constructor really executed.

---

## 25.4 Kernel piece 2 — the anonymous window and W^X

The heap cannot express permissions: every heap page is RW+X and the old
`mprotect()` over it was a no-op, so plugin code used to sit in the same pages
as `malloc`'s free lists. Phase 6 of this work added a dedicated window.

`kernel/kernel/include/mm/mmu.h` reserves a per-process VA range below the
kmalloc-VM window, **only under `#ifdef __aarch64__`**:

```c
#define DL_WINDOW_SIZE           (256*MB)          /* 0 on non-aarch64 */
#define DL_WINDOW_BASE           (KMALLOC_VM_BASE - DL_WINDOW_SIZE)
#define DL_WINDOW_END            (KMALLOC_VM_BASE)
#define DL_WINDOW_SLOT           EWOK_MMAP_ANON_ALIGN   /* 64 KB */
#define DL_WINDOW_SLOTS          (DL_WINDOW_SIZE / DL_WINDOW_SLOT)   /* 4096 */
#define DL_WINDOW_PAGES_PER_SLOT (DL_WINDOW_SLOT / PAGE_SIZE)
```

Three syscalls operate on it (`kernel/kernel/src/svc.c` →
`kernel/kernel/src/mm/mmu.c`), appended just before `SYS_CALL_NUM` so no
existing number shifts:

| Syscall | Kernel entry | Behaviour |
|---|---|---|
| `SYS_MMAP_ANON(size, prot)` | `dl_window_alloc` | first-fit over a per-process slot bitmap, `kalloc_page`s + zeroes + maps each page with `prot`; returns a window VA, or **0** when absent/exhausted |
| `SYS_MPROTECT(vaddr, size, prot)` | `dl_window_protect` | re-permissions whole pages already in the window, then `flush_tlb()` |
| `SYS_MUNMAP(vaddr, size)` | `dl_window_free` | `unmap_page_ref` (frees the page at refcount 0) and clears the slot bits |

Two details matter for correctness:

- **W^X is real.** `dl_map_page` maps with the requested AP bits and then, for a
  non-executable mapping, calls `arch_mark_pte_noexec`. This ordering is forced
  by `set_pte_flags`: on AArch64 v8 a reused PTE slot keeps the execute-never
  bit of whatever was mapped there last, so the noexec half must be applied
  *after* `map_page` — the same trick the user-stack path uses.
- **The window is released on exit.** `dl_window_release(space)` runs in the
  `TASK_TYPE_PROC` branch of `proc_funeral`, before `free_page_tables`, so a
  process that dies — normally *or* by a fault — with objects still mapped
  returns every slot and page to the allocator. All exit routes funnel through
  `proc_funeral` once `space->refs` reaches 0, and `proc_clone`/`kfork` deep-copy
  the window via `dl_window_clone` (fresh pages, exact permissions — never
  copy-on-write, which would either fault or silently undo W^X).

`EWOK_PROT_*` in `syscalls.h` are defined equal to `<sys/mman.h>`'s `PROT_*`, so
libc can pass `prot` straight through. Every `dl_window_*` call site is guarded
by `#if DL_WINDOW_SLOTS > 0`, so non-AArch64 kernels still link (the definitions
compile away and `SYS_MMAP_ANON` returns 0).

---

## 25.5 Host piece — the ABI table a static binary does not have

A plugin calls `printf` and maybe the host's own `host_callback`. Neither is in
the plugin. On Linux they would resolve against the executable's `.dynsym`;
here the host publishes them two ways (see [`dl/dl.h`](../../system/basic/libs/dl/include/dl/dl.h)):

**1. A build-time generated table.** [`gen_export.sh`](../../system/basic/libs/dl/gen_export.sh)
`nm`s the archives named by `EWOK_LIBC` (`libewoksys.a`, `libc.a`, `libgloss.a`)
and emits a C array of `(name, address)` pairs sorted by `strcmp`:

```c
extern const char SYM_printf[] __asm__("printf");   /* real asm name, unique C id */
…
const dl_symbol_t __ewok_dl_abi[] = {
    { "printf", (uintptr_t)(void *)SYM_printf },
    …
};
const uint32_t __ewok_dl_abi_count = …;   /* ~730 on machine.virt */
```

The references are emitted **strong, not weak** — and that is the whole
mechanism. An undefined *weak* symbol does not make `ld` extract an archive
member, so a weak table would export only what the host happened to use itself.
Strong references force `ld` to pull in exactly the members a plugin might call
when the app links `-ldl`. The `__asm__("printf")` label trick gives each
declaration the symbol's real assembler name while keeping the C identifier
unique, which avoids colliding with GCC's `printf` builtin. `dl_abi.c` binary
searches the sorted table.

**2. A runtime registry.** `dl_host_publish(name, addr)` /
`dl_host_unpublish(name)` let an application export symbols it *owns* (its
callbacks, a widget tree, anything from a library outside `EWOK_LIBC` such as
libm) with no build-system change. `dl_host_lookup(name)` searches the registry
first, then the static table. Both land in the same scope, ahead of any loaded
object, exactly where a Linux plugin would see the main executable.

The table is versioned: `EWOK_DL_ABI_VERSION` (a macro baked into the plugin at
compile time) and `dl_host_abi_version()` (the running host's value, itself in
the core table), so a plugin can refuse a host that is too old instead of
dereferencing an unexpected NULL.

---

## 25.6 libdl — parsing, mapping, relocating

[`dl_elf.c`](../../system/basic/libs/dl/src/dl_elf.c) is the loader's engine.

**Parse.** It validates the `ET_DYN` header, walks the program headers for
`PT_LOAD`/`PT_DYNAMIC`, and reads the dynamic table. `DT_HASH` is **mandatory**:
the SysV hash table's `nchain` is the only way to derive the `.dynsym` length,
so a GNU-hash-only object is rejected. Every struct it strides by a file-supplied
size (`DT_SYMENT`, `DT_RELAENT`) is size-asserted in
[`elf_dyn.h`](../../system/basic/libs/elf/include/elf/elf_dyn.h) — notably
`Elf64_Sym` is exactly **24 bytes** (`st_name` is an `Elf64_Word`, not 64-bit);
getting that one field wrong shifts every later field onto the wrong bytes and
shows up much later as a bogus "undefined symbol".

**Map.** The whole load span is mapped from the window and each `PT_LOAD`
segment is `memcpy`d to its `p_vaddr` (ELF congruence
`(base + p_vaddr) mod PAGE_SIZE == p_offset mod PAGE_SIZE` is checked; AArch64
`-shared` emits `p_align = 0x10000`, so the base is 64 KB-aligned to match the
slot granularity). `DT_TEXTREL` is rejected — a plugin must be position
independent.

**Relocate.** `R_AARCH64_RELATIVE` (1027) fixes up `.init_array`/`.fini_array`
and other base-relative pointers, `R_AARCH64_GLOB_DAT` (1025) fills data GOT
slots, and `R_AARCH64_JUMP_SLOT` (1026) fills PLT slots. Undefined symbols are
resolved by walking the search scope (§25.7). Binding is always eager, so an
unresolvable symbol fails at `dlopen()` with a readable `dlerror()` rather than
at first call.

**Protect (W^X pass).** After `__clear_cache`, `dl_elf_protect` re-`mprotect`s
each recorded `PT_LOAD` to its own permission — text becomes R+X, data RW —
using the real kernel page size from `getpagesize()` (16 KB on `machine.virt`,
which now reports the true value via `sys_info` rather than a hard-coded 4096).

**Init/fini.** EwokOS's own `_start` never runs `.init_array`, so `dl_run_init`
is the only thing that calls a plugin's constructors — in dependency order
(dependencies first), with destructors run dependent-first on unload.

---

## 25.7 Object graph, scope, and search path

[`dl.c`](../../system/basic/libs/dl/src/dl.c) manages loaded objects. Each
`dlopen` handle bumps a reference count; the same soname opened twice returns
the same object. `DT_NEEDED` entries are loaded recursively before the dependent
object is relocated, so cross-object symbols resolve. A bare name is treated as
a soname and searched along `DL_DEFAULT_DIRS` = `/lib:/usr/lib:/system/lib`
(there is no rpath in the EwokOS rootfs); an absolute path is used verbatim, and
`/tmp/libfoo.so` never aliases `/lib/libfoo.so` just because basenames match.

Symbol lookup builds a scope per requester: the host ABI first, then
`RTLD_GLOBAL` objects, then the requester's own dependency tree. `dlopen(NULL)`
returns a handle for the host scope, so `dlsym` can look host symbols up.
`dladdr` maps an address back to its object and nearest symbol; it returns 0 for
host code, because the static host belongs to no object.

---

## 25.8 libc routing

`system/basic/libc/libgloss/compat.c` routes the memory calls to the new
syscalls, with a fallback so nothing breaks on a target without a window:

- `getpagesize()` returns the real kernel page size from `sys_info`.
- `mmap(NULL, len, prot, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)` calls
  `SYS_MMAP_ANON`; a return of 0 means "no window" and libc falls back to the
  old zero-filled heap emulation. A registry remembers which addresses came from
  the window so `munmap` routes correctly.
- `mprotect`/`munmap` call `SYS_MPROTECT`/`SYS_MUNMAP` for window addresses and
  no-op / heap-free otherwise.

---

## 25.9 Building and loading a plugin

A plugin is compiled `-fPIC` and linked `-shared -nostdlib`
(`EWOK_SO_FLAGS` in `make.rule`) with a soname; the host links
`EWOK_LIB_DL = -ldl $(EWOK_LIBC)` (`-ldl` first, so its strong references pull
the archive members a plugin might call). Plugins install into the rootfs `/lib`
— nothing else in the tree puts anything there, so `bin/dltest/Makefile` creates
it and `make sd` packs it with `e2cp -d`.

```c
/* plugin.c — a minimal object */
#include <stdio.h>                 /* printf resolves against the host ABI table */
int demo_entry(int x) { return x + 1; }
__attribute__((constructor)) static void init(void) { printf("demo: ctor\n"); }
```

```c
/* host.c */
#include <dlfcn.h>
void* h = dlopen("/lib/libdemo.so", RTLD_NOW);
int (*fn)(int) = (int (*)(int))dlsym(h, "demo_entry");
if(fn) printf("demo_entry(41) = %d\n", fn(41));
dlclose(h);
```

Deliberate deviations from glibc/musl, all documented in
[`dlfcn.h`](../../system/basic/libs/dl/include/dlfcn.h): `RTLD_LAZY` is accepted
but binding is always eager; `RTLD_NEXT` is refused by `dlsym()` (`RTLD_DEFAULT`
works and searches the global scope); `dlerror()` is per-process, not per-thread.

---

## 25.10 The self-test: what "94 checks, 0 failures" proves

[`bin/dltest`](../../system/basic/bin/dltest/) is written to be *evidence*, not a
demo. It prints a machine-greppable verdict and is run at boot on `machine.virt`.
Among the plugins it ships: `libdlplug_a.so` (exercises every relocation type,
`.init_array`/`.fini_array`, an intra-object PLT call, a host callback, and
`dl_host_lookup`), `libdlplug_b.so` (`DT_NEEDED` on A, cross-object JUMP_SLOT and
GLOB_DAT), `libdlplug_bad.so` (an unresolvable symbol, to prove the error path),
and `libdlplug_m1..m8.so` (one source compiled with a different `PLUG_ID` each).

What the passing run demonstrates:

- **Cache sync works** — `A: ctor ran (.init_array)` prints during `dlopen`.
- **The host ABI table resolves** `printf`/`puts`/`malloc` from a plugin, and the
  registry resolves `host_callback` both directions.
- **All three relocation types apply with the right bias**, dependencies load,
  sonames dedup to one shared instance, and init/fini order is dependent-first.
- **W^X is enforced** — the separate `wxfault` invocation stores into a plugin's
  read-only text and takes a real data abort (`ESR 0x9200004f` = permission
  fault, level 3, write), and the kernel kills the process.
- **Many objects + release** — section 14 opens eight distinct objects at once
  (eight distinct load bases, each running its own relocated code and data),
  closes them all (`dl_object_count()` back to 0), and reopens a fresh set, which
  only succeeds if the window slots *and* their backing pages were really freed.
- **Exit release** — a `leak` invocation leaves four objects mapped and exits
  without `dlclose`; a full suite run right after it (and after the faulting
  `wxfault`) still passes, which is the evidence that `proc_funeral` reclaimed the
  window on both a leaking normal exit and an abnormal one without corrupting the
  allocator.

---

## 25.11 Limitations

- The W^X window is **AArch64-only**; other architectures fall back to heap
  emulation (RWX, no per-page permissions), and the x86 toolchain does not even
  link the plugin `.so` against the undefined host symbols.
- No lazy binding, no copy relocations (a plugin reading a host *variable* is
  rejected; the host reading a plugin variable via `dlsym` works), no `RTLD_NEXT`
  interposition chain, no plugin TLS.
- `dlerror()` is per-process, so two threads failing concurrently can see each
  other's message.

---

## See also

- [05 Memory & MMU](05-mmu.md) — the VA layout the window slots into
- [07 Processes](07-process.md) — `proc_funeral` and the exit-time release
- [08 System Calls](08-syscall.md) — how the four new syscalls are dispatched
- [23 Cache Maintenance](23-cache.md) — PoU, clean/invalidate, why `SYS_CACHE_SYNC` exists
- [16 Library Overview](16-libs-overview.md) — where `libdl` fits in the library stack
- [99 Glossary](99-glossary.md)
