# 23 Cache Maintenance: How the Kernel Keeps Memory Coherent

> Language: **English** | [中文](23-cache.zh.md)
>
> Goal: understand *why* a CPU cache makes an operating system's job harder;
> learn the exact vocabulary (clean / invalidate / flush, PoC / PoU, by-set-way
> vs by-VA); read EwokOS's portable cache API and its AArch64 / ARMv7 / x86
> implementations line by line; and see the four real situations where the
> kernel *must* refresh a cache or a TLB — changing a mapping, switching an
> address space, loading code, and handing memory to a device.
>
> This chapter is a deep dive that sits beside [05 Memory & MMU](05-mmu.md)
> and [07 Processes](07-process.md). Terminology (TLB/TTBR/ASID/PoC/PoU/…) is
> collected in the [glossary](99-glossary.md).

Chapters [05](05-mmu.md) and [07](07-process.md) quietly assumed that "when the
kernel writes a byte, everybody sees it." On real hardware that is **false**,
and the reason it is false is the cache. This chapter removes that assumption
and shows exactly what EwokOS does to keep the CPU, the instruction fetcher,
the page-table walker, and the DMA devices all agreeing about what memory
contains.

Every code reference below points at a real file:

| Layer | File |
|---|---|
| Portable API (declarations) | [`kernel/kernel/include/kernel/system.h`](../../kernel/kernel/include/kernel/system.h) |
| Platform-independent helpers (delay, kernel lock, halt) | [`kernel/kernel/src/system.c`](../../kernel/kernel/src/system.c) |
| Per-arch C wrappers (policy) | `kernel/platform/<arch>/arch/<ver>/system_arch.c` — [aarch64/v8](../../kernel/platform/aarch64/arch/v8/system_arch.c), [arm/v7](../../kernel/platform/arm/arch/v7/system_arch.c), [x86/x64](../../kernel/platform/x86/arch/x64/system_arch.c), [riscv/rv64](../../kernel/platform/riscv/arch/rv64/system_arch.c) |
| AArch64 assembly (mechanism) | [`kernel/platform/aarch64/arch/v8/system.S`](../../kernel/platform/aarch64/arch/v8/system.S) |
| ARMv7 assembly | [`kernel/platform/arm/arch/v7/system.S`](../../kernel/platform/arm/arch/v7/system.S) |
| x86 assembly | [`kernel/platform/x86/arch/x64/system.S`](../../kernel/platform/x86/arch/x64/system.S) |
| Callers | `mm/mmu.c`, `mm/shm.c`, `mm/dma.c`, `proc.c`, `svc.c`, `irq.c` |

> **Recent refactor ("kernel arch porting friendly")**: every cache / TLB /
> address-space primitive used to live in one shared `system.c` behind
> `#if defined(__aarch64__) / ARM_V7 / …`. Each of those branches has been
> moved out, verbatim in behaviour, into a per-arch
> `kernel/platform/<arch>/arch/<ver>/system_arch.c` (wired into the build via
> each platform's `make.rule`). The shared `kernel/kernel/src/system.c` now
> contains only platform-independent helpers (`_delay*`, `kernel_lock*`,
> `halt`) and carries **no per-arch conditional compilation at all** — which
> is what makes porting to a new architecture a matter of adding one new
> `system_arch.c` instead of touching shared code.

## 23.1 Why a Cache Creates a Problem for the Kernel

A modern CPU reads memory in **cache lines** (typically 64 bytes) and keeps a
copy in a small, fast SRAM cache. When you write, the write usually lands in
the cache and *not* in DRAM; the dirty line is written back later, whenever the
cache feels like it. This is invisible — and harmless — for ordinary program
data, because the same core reads its own writes through the same cache.

It becomes a problem the moment a **second observer** looks at the same physical
memory through a *different* path:

1. **The instruction fetcher.** The I-cache is separate from the D-cache. Code
   the kernel just wrote into a page (an ELF load) sits in the D-cache; the
   I-cache still holds whatever *used* to be at that physical address — often
   the instructions of a previous, now-dead process whose page was recycled.
2. **The page-table walker.** The MMU reads page tables to translate addresses.
   If the walker reads page tables from DRAM (a "non-cacheable walk") while the
   kernel's updated PTE is still dirty in the D-cache, the walker translates
   using a stale entry.
3. **A DMA device / the GPU.** A device reads DRAM directly. If the CPU wrote a
   buffer but the line is still dirty in cache, the device reads garbage; and
   when the device writes DRAM, a stale clean line in the CPU cache can later be
   evicted and overwrite the device's data.
4. **Another CPU core.** In SMP, core B's cache may hold a line that core A just
   changed. Cache *maintenance* operations must therefore be **broadcast** to
   the whole inner-shareable domain, not just done locally.
5. **The TLB.** Not a data cache, but the same class of problem: the MMU caches
   *translations*. After you change a PTE, the old translation is still cached
   in the TLB and must be explicitly invalidated.

So "refreshing the cache" is really five related chores. The rest of the chapter
is a map of how EwokOS performs each one, on each architecture.

## 23.2 The Vocabulary You Must Know First

ARM (and this tutorial) use three verbs and two "points" precisely. Getting them
mixed up is the #1 source of subtle bugs.

| Term | Meaning | Effect on a dirty line |
|---|---|---|
| **Clean** | Write dirty data back to the next level, **keep** the line in cache | Data reaches memory, line stays cached (now clean) |
| **Invalidate** | Throw the line away **without** writing it back | Dirty data is **lost** |
| **Flush** (= clean + invalidate) | Write back **and** drop the line | Data reaches memory, line is gone |

And the two "points" a maintenance operation targets:

- **PoU (Point of Unification)** — the point where the D-cache, the I-cache and
  the MMU walker all agree. Cleaning code to PoU makes it visible to instruction
  fetch.
- **PoC (Point of Coherency)** — the point where *all* observers (CPU, DMA,
  other cores) see the same bytes. Flushing data to PoC makes it visible to a
  device.

Finally, *how* you address the lines:

- **By set/way** — walk every line of every cache level geometrically. This is
  the only way to hit the **whole** cache; it is slow and it destroys the
  working set. Used at boot and for a full publish.
- **By VA (by virtual address)** — operate on one cache line at a time by
  address. Cheap and surgical. This is what the hot paths use.

> Rule of thumb: **by-VA for a known range, by-set-way only when you truly mean
> "everything".** A large part of EwokOS's recent optimization work was replacing
> whole-cache by-set-way sweeps with targeted by-VA operations.

## 23.3 The Portable API

All callers use one portable header, [`kernel/kernel/include/kernel/system.h`](../../kernel/kernel/include/kernel/system.h).
It declares a small, arch-independent set:

```c
extern void flush_tlb(void);              // publish mapping changes globally
extern void flush_tlb_nosweep(void);      // TLB-only invalidate (descriptors already published)
extern void flush_tlb_addr(ewokos_addr_t);// invalidate one page's TLB entry
extern void flush_tlb_asid(uint32_t asid);// drop every TLB entry tagged with one ASID
extern void flush_dcache(void);           // clean+invalidate the WHOLE D-cache (by set/way)
extern void invalidate_dcache(void);      // invalidate the whole D-cache (no write-back)

/* code-load coherency: clean written code to PoU, then drop the I-cache */
extern void dcache_clean_code_range(const void* start, uint32_t size);
/* push data written through a cacheable alias out to DRAM (PoC) and drop it */
extern void dcache_flush_range(const void* start, uint32_t size);
extern void invalidate_icache_all(void);  // drop the I-cache (broadcast on SMP)
```

These are **not** the raw instructions. Each one is implemented per
architecture in
`kernel/platform/<arch>/arch/<ver>/system_arch.c` (e.g.
[`aarch64/v8/system_arch.c`](../../kernel/platform/aarch64/arch/v8/system_arch.c),
[`arm/v7/system_arch.c`](../../kernel/platform/arm/arch/v7/system_arch.c)) as a
thin C wrapper over the assembly primitives in the neighbouring `system.S`.
The wrappers decide, for their own architecture, the correct behaviour —
including whether a full D-cache sweep is even necessary. They are the key
design decision: **the *policy* ("does a mapping change need a whole-cache
clean?") lives in per-arch C; only the *mechanism* (the instruction) lives in
assembly.** The shared `system.c` no longer participates in this at all.

## 23.4 `flush_tlb()`: The Most Delicate Wrapper

`flush_tlb()` is called whenever page tables change in bulk. Its body differs
dramatically by architecture, and reading it is the fastest way to understand
each platform's memory model.

**AArch64** ([`v8/system_arch.c`](../../kernel/platform/aarch64/arch/v8/system_arch.c)):

```c
void flush_tlb(void) {
    /* Page tables are walked through the D-cache (TCR IRGN/ORGN write-back,
     * inner shareable) and user memory is PIPT-coherent, so publishing PTE
     * stores only needs the dsb inside __flush_tlb. */
    __flush_tlb();
}
```

**ARMv7** ([`v7/system_arch.c`](../../kernel/platform/arm/arch/v7/system_arch.c)):

```c
void flush_tlb(void) {
    /* Bulk table construction and board-level page-table copies still rely
     * on a whole D-cache publish; ... */
    flush_dcache();
    __invalidate_icache_all();
    __flush_tlb();
}
```

**x86 / RISC-V / ARMv5 / ARMv6** each carry their own variant in their own
`system_arch.c` (x86 and RISC-V: `flush_dcache()` + `__flush_tlb()`, plus an
I-cache drop under `KERNEL_SMP`).

On AArch64 the *entire* D-cache sweep was **removed**. Why is that safe? Because
the boot code programs `TCR_EL1` so that the page-table walker reads tables as
**inner-shareable, write-back cacheable** memory, and user memory is
**PIPT** (physically-indexed-physically-tagged) coherent. The walker therefore
*snoops* the D-cache and always sees the newest PTE. All that remains is a data
synchronization barrier (`dsb`) to order the store, then the TLB invalidate.

On ARMv7 the whole-D-cache clean is **kept** for the bulk path. The reasoning is
in §23.6.

The AArch64 assembly it lands on
([`v8/system.S`](../../kernel/platform/aarch64/arch/v8/system.S)) is just four
instructions:

```asm
__flush_tlb:
    dsb  ishst          // publish prior PTE stores to the walker
    tlbi vmalle1is      // invalidate all EL1&0 entries, inner-shareable broadcast
    dsb  ish            // wait for the invalidation to complete
    isb                 // resynchronize instruction fetch
    ret
```

Read the mnemonic: `tlbi vmalle1is` = **TLB Invalidate**, "all entries for the
current VMID at EL1&0", **IS** = broadcast to the **I**nner **S**hareable
domain. The `is` suffix is what makes it correct on SMP.

## 23.5 The Single-Page Fast Paths

Bulk `flush_tlb()` is a cannon. Most mapping changes touch **one page**, so
EwokOS has two surgical wrappers, both used heavily by `proc.c`, `shm.c` and
`irq.c`.

### `flush_tlb_addr(addr)` — invalidate one page

AArch64 ([`v8/system_arch.c`](../../kernel/platform/aarch64/arch/v8/system_arch.c)):

```c
void flush_tlb_addr(ewokos_addr_t addr) {
    ewokos_addr_t page = addr >> 12; /* TLBI VA operand: VA[55:12], granule-independent */
    __asm__ volatile(
        "dsb ishst\n"
        "tlbi vaae1is, %0\n"          // all-ASID form: user pages are nG
        "dsb ish\n"
        "isb\n"
        :: "r"(page) : "memory");
}
```

ARMv7 ([`v7/system_arch.c`](../../kernel/platform/arm/arch/v7/system_arch.c)):

```c
void flush_tlb_addr(ewokos_addr_t addr) {
    ewokos_addr_t page = addr & ~(ewokos_addr_t)0xfff; // VA[31:12] IN PLACE
    __asm__ volatile(
        "dsb\n"
#ifdef KERNEL_SMP
        "mcr p15, 0, %0, c8, c3, 3\n" /* TLBIMVAAIS */
#else
        "mcr p15, 0, %0, c8, c7, 3\n" /* TLBIMVAA */
#endif
        "dsb\n"
        "isb\n"
        :: "r"(page) : "memory");
}
```

The remaining architectures in their own `system_arch.c`: ARMv5/v6 fall back
to the full `flush_tlb()` (no per-line table publish there); RISC-V issues
`sfence.vma addr`; x86 issues `invlpg (addr)`.

Two details here are load-bearing and were each the source of a real bug:

- **The AArch64 TLBI VA operand is `addr >> 12`, not `addr >> PAGE_SHIFT`.** ARM
  defines the operand as VA[55:12] *regardless of the configured granule*; the
  hardware extracts the right bits itself. On a 16 KB-granule platform (raspi5)
  using `>> PAGE_SHIFT` (=14) computes the wrong operand and leaves stale TLB
  entries → a silent translation fault. Never "fix" this shift.
- **AArch64 uses the all-ASID form `vaae1is`**, because user pages are marked
  `nG` (non-global) and tagged with an ASID; the plain `vae1is` form would only
  match entries whose ASID equals the zero top bits of the operand and would
  miss everything.
- On **ARMv7 the operand is the VA with the low 12 bits cleared, *in place*** —
  the opposite convention from AArch64. Do not port one to the other.

### `flush_tlb_nosweep()` — TLB invalidate only

When `map_page`/`unmap_page` have *already* published the changed descriptor to
PoC (ARMv7 does this per-line) or the walk is PIPT-coherent (AArch64), a caller
that only touched data mappings can skip the whole-cache sweep entirely:

```c
/* aarch64 & arm/v7 system_arch.c: */
void flush_tlb_nosweep(void) {
    __flush_tlb();     // TLBI only; descriptors already visible to the walker
}

/* arm/v5, arm/v6, riscv, x86 system_arch.c: */
void flush_tlb_nosweep(void) {
    flush_tlb();       // no per-line publish path: fall back to the full form
}
```

`proc.c` uses this for heap/stack growth and copy-on-write remaps
(`proc_expand_mem`, `proc_shrink_mem`, fork COW). It is valid **only** for data
mappings that no non-coherent master reads through a cacheable alias — never for
bulk table copies or the exec code path.

## 23.6 ARMv7: The Cacheable Page-Walk Refactor

ARMv7 (`raspix`, `machine.virt` arm32) originally walked page tables as
**non-cacheable**: `__set_translation_table_base` left `TTBR0`'s IRGN/RGN/S bits
at zero. That meant the walker read tables straight from DRAM. But the kernel
writes PTEs through its *cacheable* linear mapping (`P2V`), so a dirty PTE line
could sit in the D-cache and the walker would never see it — hence the need for
a **whole-D-cache clean** on every mapping change, as a blunt safety net.

The refactor (whose `-DARM_V7` macro in
[`kernel/platform/arm/make.rule`](../../kernel/platform/arm/make.rule) still
tells the v7 assembly apart from v5/v6, and whose C policy now lives in the
dedicated [`v7/system_arch.c`](../../kernel/platform/arm/arch/v7/system_arch.c)
rather than in shared code) turned the walk cacheable and
inner-shareable, exactly like AArch64:

```asm
__set_translation_table_base:
    dsb
    mcr p15, 0, r0, c2, c0, 0     // TTBR0 = base  (base already OR'd with 0x0B)
    isb
    ...
```

The `0x0B` is `IRGN=write-back (bit0) | S=shareable (bit1) | RGN=write-back
(bit3)`. With that, the walker snoops a coherent D-cache across cores. `boot.S`'s
`load_boot_pgt` sets the same bits so the boot page table walks identically.

Even so, ARMv7 **keeps** the switch-time whole-D-cache sweep. The comment in
`set_translation_table_base_asid()` is blunt about why:

```c
/* Keep the switch-time whole-D-cache sweep: on ARMv7 it is load-bearing
 * for non-coherent DMA/graphics doorbell ordering (removing it freezes
 * X on real boards; an I-cache-only or bare-dsb variant does not
 * substitute). */
flush_dcache();
__set_translation_table_base_asid(tlb_base, asid);
```

This is a hard-won, real-hardware fact: on ARMv7 the sweep is *not* about TLB or
walker correctness — it is what makes non-coherent DMA and the VC4 graphics
doorbell behave. Removing it hangs the X window system on a real board.

The by-VA building blocks ARMv7 gained in the same refactor
([`v7/system.S`](../../kernel/platform/arm/arch/v7/system.S)):

```asm
__dcache_flush_poc_range:            // clean+invalidate a VA range to PoC
    ...  mcr p15, 0, r0, c7, c14, 1  // DCCIMVAC, line size from CTR.DminLine

__dcache_clean_pou_range:            // clean code to PoU
    ...  mcr p15, 0, r0, c7, c11, 1  // DCCMVAU

__invalidate_icache_all_is:          // broadcast I-cache invalidate
    mcr p15, 0, r0, c7, c1, 0        // ICIALLUIS
```

ARMv5/v6 ([`v5/system_arch.c`](../../kernel/platform/arm/arch/v5/system_arch.c),
[`v6/system_arch.c`](../../kernel/platform/arm/arch/v6/system_arch.c)) keep the
original semantics: a non-cacheable walk, whole-cache maintenance inside
`flush_tlb()`, and *empty* `dcache_flush_range` / `dcache_clean_code_range`
wrappers.

## 23.7 x86: MESI Makes Most of This Unnecessary

x86 hardware is cache-coherent by design (MESI protocol + PCIe snooping). A DMA
device's accesses snoop the CPU caches automatically, so the software
clean/invalidate dance that ARM needs **does not exist** on x86. EwokOS reflects
that in [`kernel/platform/x86/arch/x64/system.S`](../../kernel/platform/x86/arch/x64/system.S):
`__flush_dcache_all` / `__invalidate_dcache_all` are **not** `wbinvd`; they were
deliberately degraded to a plain `mfence` barrier.

Why not `wbinvd`? Because `wbinvd` flushes the *entire* cache — hugely expensive
— and inside a VM it triggers a costly VM-exit. A barrier is enough for coherent
memory. What x86 *does* still need:

- **TLB**: reload `CR3` (or `invlpg` for one page) after a mapping change —
  x86 does not auto-invalidate the TLB.
- **I-cache / store ordering**: `mfence` where the kernel wrote code.
- **MTRR/PAT changes**: `x86_pat_init` still issues a real `wbinvd` once, because
  changing the memory-type registers genuinely requires a full flush.

## 23.8 Where the Kernel Actually Calls All This

The wrappers mean nothing until you see the four situations that invoke them.

### (a) Changing a mapping — `mmu.c`, `shm.c`

`map_page` / `unmap_page` write a PTE. Every caller then invalidates the
affected translation. `shm.c` scopes it to single pages:

```c
unmap_page(_kernel_info.kernel_vm, addr);
flush_tlb_addr(addr);   // scope invalidation to just this page
```

Bulk remaps (kmalloc VM arena in `kmalloc_vm.c`, DMA peer teardown in `dma.c`)
still use the full `flush_tlb()`.

### (b) Switching address space — `set_translation_table_base_asid()`

This runs on **every process switch**. On AArch64 it is nearly free:

```c
if(asid != 0 && asid < asid_limit()) {
    __set_translation_table_base_asid(tlb_base, asid);  // no TLBI at all
    return;
}
```

Every space carries its own **ASID** and its user PTEs are `nG`, so loading
`TTBR0` with `base | (asid<<48)` is the *whole* TLB side of the switch — the
previous space's entries simply stop matching and stay warm. No invalidate. Only
a space whose ASID does not fit the core's width (or `asid==0`) falls back to a
local full invalidate. `asid_limit()` reads `ID_AA64MMFR0_EL1.ASIDBits` to
support both 8-bit and 16-bit ASID cores rather than assuming.

When an ASID is *recycled* (a pde slot reused), `flush_tlb_asid()` drops the old
owner's tagged entries on all cores so a reused number never resolves through
leftovers.

### (c) Loading code / `exec` — `proc.c`

This is the I-cache scenario. When the kernel copies an ELF segment into a fresh
(reused) physical page, it must make the new instructions visible to fetch:

```c
if(exec)
    dcache_clean_code_range((const void*)kaddr, chunk);  // clean code to PoU
...
invalidate_icache_all();   // drop the I-cache (broadcast on SMP)
```

Note the **order**: clean the data side first, *then* invalidate the I-cache.
Reversing it would let a stale line refill between the two steps. And on SMP the
invalidate must broadcast (`__invalidate_icache_all_is`, `ICIALLUIS` /
`IC IALLUIS`) because the new process may be scheduled onto a *different* core
that still holds the old page's instructions.

### (d) Handing memory to a device — `shm.c`, `dma.c`

The DMA/shared-memory scenario. Contiguous shared memory is mapped **twice**: a
cacheable alias the CPU uses to initialise it, and a `NOCACHE` alias that the
device (or a non-coherent master) reads. Before the device touches it, the CPU's
dirty lines must reach DRAM and be dropped:

```c
/* Clean the cacheable alias out to DRAM and drop the lines, or a later
 * eviction would overwrite what the GPU/CPU write through the NOCACHE alias */
dcache_flush_range((void*)P2V(paddr), pages * PAGE_SIZE);
```

The subtle corollary, documented right in `shm.c`: **the process-visible mapping
of device-shared memory must itself be `NOCACHE`.** Previously the switch-time
whole-D-cache clean accidentally hid a cacheable contiguous mapping; once the
per-switch sweep was removed on AArch64, that aliasing had to be fixed properly
by making both views non-cacheable. This is exactly the kind of "hidden side
effect" that §23.10 warns about.

## 23.9 SMP: Why the `is` Suffix Matters

On a single core, `tlbi vmalle1` and `ic iallu` suffice. On SMP they do not: a
maintenance op run locally leaves the *other* cores' TLBs and I-caches stale.
ARM provides **inner-shareable broadcast** forms, and EwokOS selects them under
`#ifdef KERNEL_SMP` / by arch:

| Operation | Local form | Broadcast form (used on SMP) |
|---|---|---|
| TLB invalidate all | `tlbi vmalle1` | `tlbi vmalle1is` |
| TLB invalidate by ASID | — | `tlbi aside1is` |
| I-cache invalidate | `ic iallu` | `ic ialluis` |
| ARMv7 TLB all | `TLBIALL (c8,c7,0)` | `TLBIALLIS (c8,c3,0)` |
| ARMv7 I-cache | `ICIALLU (c7,c5,0)` | `ICIALLUIS (c7,c1,0)` |

The barriers pair with the domain too: `dsb ish` (inner-shareable) for broadcast
ops, `dsb nsh` (non-shareable) for the local-only fallback in
`__flush_tlb_local`.

## 23.10 Hard-Won Lessons (Read This Before Optimizing)

These are real bugs from EwokOS's development. Each one is a reason to be
careful:

1. **Never change the AArch64 TLBI VA shift from `addr >> 12` to
   `addr >> PAGE_SHIFT`.** The operand is defined as VA[55:12] independent of
   granule; on 16 KB/64 KB pages the "obvious fix" computes a wrong address,
   leaving silent stale TLB entries and translation faults at boot.
2. **Before deleting any global cache/TLB maintenance, inventory its side
   effects.** The exec path *depended* on the I-cache invalidate that used to
   ride along inside the old SMP `flush_tlb()`. When `flush_tlb()` was slimmed to
   a pure TLBI, the I-cache drop had to be re-added explicitly in `proc.c`
   (`dcache_clean_code_range` + `invalidate_icache_all`), or new processes crash
   on stale instructions.
3. **On ARMv7, the switch-time whole-D-cache sweep is load-bearing for
   non-coherent DMA/graphics** — not for TLB correctness. Removing it freezes the
   window system on a real board. An I-cache-only or bare-`dsb` variant does not
   substitute.
4. **QEMU does not model cache coherence.** It has no real L1/L2/I-cache, treats
   aliases as always coherent, walks page tables straight from RAM, and does not
   model ASID reservation. QEMU is fine for a *boot smoke test* (proving a new
   instruction encoding is not UNDEFINED and the kernel still starts), but
  screen-corruption-style aliasing bugs and stale-instruction crashes can **only** be caught on
   real hardware — run `xwin` + VC4 g2d for aliasing, and heavy fork/exec for
   I-cache broadcast.
5. **Cacheable ↔ non-cacheable aliasing is a correctness invariant, not a
   performance choice.** If a physical frame is reachable through both a
   cacheable and a non-cacheable mapping, the cacheable side must be flushed to
   PoC before the non-cacheable side is trusted — otherwise the CPU reads stale
   DRAM or a later eviction clobbers a device's write.

## 23.11 Summary

- A cache makes "write once, seen everywhere" false. The kernel must actively
  maintain coherence across five observers: D-cache, I-cache, the page-table
  walker, DMA devices, and other cores.
- **Clean** writes back and keeps; **invalidate** drops without write-back;
  **flush** does both. Target **PoU** for code, **PoC** for device data. Prefer
  **by-VA** over **by-set-way**.
- EwokOS puts the *policy* in per-arch C wrappers
  (`kernel/platform/<arch>/arch/<ver>/system_arch.c`) and only the *mechanism*
  in the neighbouring per-arch assembly (`system.S`); the shared
  [`system.c`](../../kernel/kernel/src/system.c) keeps just the
  platform-independent helpers and no arch `#if`s at all.
- AArch64 and (after refactor) ARMv7 walk page tables **cacheable +
  inner-shareable**, so `flush_tlb()` collapses to a broadcast TLBI plus a `dsb`.
  ARMv7 still keeps a whole-D-cache sweep at switch time for DMA/graphics.
  x86 needs almost nothing thanks to MESI — `wbinvd` was degraded to `mfence`.
- Four real call sites: mapping change (`flush_tlb_addr` / `flush_tlb_nosweep`),
  address-space switch (ASID, no TLBI on AArch64), code load
  (`dcache_clean_code_range` + `invalidate_icache_all`), device handoff
  (`dcache_flush_range`).
- On SMP, always the **broadcast (`is`)** forms.

**Next:** with memory coherence understood, [15 Debugging](15-debug.md) shows how
to attach GDB and read the logging stack when a coherence bug *does* slip
through to real hardware.
