# 05 Memory Management and the MMU

> Language: **English** | [中文](05-mmu.zh.md)
>
> Goal: understand virtual memory and page tables; see how EwokOS builds the
> kernel memory layout; understand the physical page allocator (kalloc) and
> the kernel heap (kmalloc); and why every process believes it owns all of
> memory. This chapter is code set #3 on the hands-on main line: fasten your
> seatbelt — from the physical world into the virtual one.
> Terminology (MMU/VA/PA/PTE/TLB/TTBR/CoW…) is in the [glossary](99-glossary.md).

This is the most central — and most exquisite — chapter about operating
systems. Read it slowly.

## 5.1 The Problem: Not Enough Memory to Share, and No Room for Sloppiness

A Raspberry Pi 3B has only 1 GB of RAM. On top of it we must run the kernel,
the filesystem, the window system, and a dozen applications. Three problems
must be solved:

1. **Isolation**: process A must not read or write process B's memory, let
   alone the kernel's;
2. **Deception**: every process believes "memory starts at 0 and it's all
   mine" — programs are written without caring about anyone else;
3. **Flexibility**: a program's memory need not be physically contiguous,
   and it should grow on demand.

The solution is the **MMU (Memory Management Unit)** + **virtual memory**.

## 5.2 The Meaning of Virtual Addresses

Before the how, a more fundamental question: **why not just use physical
addresses?**

Imagine a world without virtual addresses. You write a program, and the
linker must assign it a physical address range, say `0x100000`. The
problems:

- **Collision**: what if another program also wants `0x100000`? First come,
  first served? Whoever loses has to move house — but every absolute address
  in the code must move with it;
- **Exposure**: a program holding physical addresses can read and write
  anywhere — the kernel, other processes, hardware registers. Security is
  out of the question;
- **Fragmentation**: a program wants 10 MB contiguous. Physical memory has
  scattered free chunks, no contiguous 10 MB — the machine has plenty of
  spare memory yet must report failure.

Virtual addresses solve all three at once; at heart they are a
**contractual deception**:

> Every program uses a fake address space that "starts at 0 and is
> contiguous and vast". The hardware (MMU) secretly translates the fake
> address into the real physical location on every single access.

Three direct benefits:

| Benefit | Mechanism | Where you'll use it |
|---|---|---|
| **Isolation** | Each process has its own translation table (page table); whatever isn't mapped in your table is unreachable | Ch. 04's user processes, §5.8 |
| **Position independence** | A program always believes it lives at low addresses; physical placement is arbitrary; two programs at the "same address" don't interfere | Ch. 12: every program loads at `0x100` |
| **Non-contiguous + on-demand** | Virtually contiguous, physically scattered pieces; allocate a page only when it's touched | `malloc` expansion, fork's copy-on-write (Ch. 07) |

A fitting analogy: **a virtual address is like a hotel room number; a
physical address is the real floor coordinate**. Every guest believes
they're in "Room 301" (the illusion is consistent), and the hotel (the
kernel) holds the guest roster (the page table) deciding which real room
each number corresponds to; changing guests means changing the roster — the
rooms themselves don't move.

Remember this sentence; every detail that follows is its expansion:
**A virtual address is a "translation contract" between a process and
memory; the page table is the translation dictionary; switching processes =
swapping dictionaries.**

## 5.3 The MMU and Page Tables: a Translation Machine

The address the CPU uses for every memory access is a **virtual address
(VA)**. The MMU is a hardware module inside the CPU that **translates** the
virtual address into a **physical address (PA)** before the access really
happens:

```
program: ldr x0, [0x1000]          (virtual address)
            │
            ▼  MMU consults the page table
memory:  physical address 0x9ABF1000   (the real location)
```

The translation rules live in the **page table**, maintained by the OS. The
rules are **paged**: virtual memory is cut into fixed-size **pages** (EwokOS
uses 4 KB), and the page table states "where each page maps, with what
permissions (readable / writable / executable / is EL0 access allowed)".

This is how isolation happens: process A's page table simply contains no
mapping for kernel memory — A has no road to get there even if it tries
(the MMU throws an exception, handled by Ch. 04's exception machinery).

### AArch64's Four-Level Page Table

AArch64 translates 48-bit virtual addresses with a **4-level page table**
(directories nested in directories):

```
48-bit virtual address:
┌────────┬────────┬────────┬────────┬────────────┐
│ L0 index │ L1 index │ L2 index │ L3 index │ in-page offset │
│  9 bits  │  9 bits  │  9 bits  │  9 bits  │   12 bits    │
└────────┴────────┴────────┴────────┴────────────┘
    │         │         │         │          │
    ▼         ▼         ▼         ▼          ▼
  L0 tbl ─► L1 tbl ─► L2 tbl ─► L3 tbl ─► offset inside the 4KB physical page
```

Two key registers:
- `TTBR0_EL1`: governs the **low address space** (virtual addresses starting
  at 0 — for user processes);
- `TTBR1_EL1`: governs the **high address space** (the region whose top
  address bit is 1 — for the kernel).

Hence EwokOS's convention: **user programs at low addresses, the kernel at
high addresses**.

## 5.4 EwokOS's Memory Layout

See [mmudef.h](../../kernel/platform/aarch64/arch/common/include/mm/mmudef.h)
and [mmu.h](../../kernel/kernel/include/mm/mmu.h):

```c
#define KERNEL_BASE  0x4000000000ull   // 256GB mark: start of kernel virtual addresses
// MMIO immediately follows the kernel's usable memory region
#define MMIO_BASE    (KERNEL_BASE + MAX_USABLE_MEM_SIZE)
```

The overall layout (AArch64):

```
High addresses (TTBR1's domain, kernel-only)
┌──────────────────────────────────────────┐
│ MMIO region   device registers (UART/SD/IRQ) │ ← MMIO_BASE, mapped to device PA
├──────────────────────────────────────────┤
│ kmalloc region   kernel heap (small objects) │
├──────────────────────────────────────────┤
│ direct map of allocatable physical memory   │ ← the kernel's "RAM window"
├──────────────────────────────────────────┤
│ kernel page-directory region (all processes' │
│ page tables live here)                     │
├──────────────────────────────────────────┤
│ kernel image (code + data)                  │ ← KERNEL_BASE
└──────────────────────────────────────────┘
Low addresses (TTBR0's domain, different per process)
┌──────────────────────────────────────────┐
│ process code, data, heap, stack…            │ ← each process's own view
└──────────────────────────────────────────┘
```

Why place the kernel at high addresses? **Switching processes only requires
swapping the lower half of the page table; the upper half (the kernel) is
shared by all processes** — Ch. 07's `clone_kernel_vm` does exactly this.

## 5.5 Boot Hurdle #1: the Boot Page Table

At power-on the MMU is off, so virtual address = physical address. But the
kernel code was compiled for "high addresses (KERNEL_BASE)" — it cannot run
until the MMU is on. A classic chicken-and-egg problem. EwokOS's answer:
**first enable the MMU with the simplest possible page table, then switch
to the real one**.

See `_boot_start` in [start.c](../../machines/raspix/kernel/bsp/start.c):

```c
void _boot_start(void) {
    boot_pgt_init();
    set_boot_pgt(0, 0, 64*MB, 0);                  // ① identity-map the low 64MB
    set_boot_pgt(KERNEL_BASE, 0, 64*MB, 0);        // ② map the same 64MB at the kernel's high address
    switch(cpu_part()){                             // ③ map device registers per CPU model
        case ARM_CPU_PART_CORTEX_A72:               //    Pi4
            set_boot_pgt(MMIO_BASE, PIX4_MMIO_PHY, PIX_MMIO_SIZE, 1);
            break;
        default:                                    //    Pi2/Pi3
            set_boot_pgt(MMIO_BASE, PIX3_MMIO_PHY, PIX_MMIO_SIZE, 1);
            break;
    }
    load_boot_pgt((ewokos_addr_t)startup_page_dir); // ④ load the table, enable the MMU
}
```

Reading it:
- ① makes "VA 0~64MB = PA 0~64MB", so the code currently running doesn't
  break when the tables switch;
- ② lets the kernel reach that same physical memory at the compile-time
  high address (KERNEL_BASE);
- ③ maps the device physical addresses (`0x3F000000` on Pi3) into the
  high-half MMIO region. Note the last parameter `is_dev=1`: **device memory
  must not be cached** — every access must really happen;
- ④ `load_boot_pgt` is in
  [boot.S](../../kernel/platform/aarch64/arch/v8/boot.S):

```asm
load_boot_pgt:
    ldr x1, =MAIR1VAL
    msr mair_el1, x1     ; define memory attributes (normal memory cacheable / device memory not)
    msr ttbr0_el1, x0    ; load the page-table base address
    msr tcr_el1, x0      ; configure the table format (4KB pages, 48-bit addresses...)
    mrs x0, sctlr_el1
    ldr x1, =SCTLREL1VAL
    orr x0, x0, x1       ; M=1 (MMU on) + C=1 (data cache on) + I=1 (instruction cache on)
    msr sctlr_el1, x0
    ret                  ; from this moment, every memory access goes through translation
```

## 5.6 The Real Kernel Page Table

The boot table was only an emergency measure. `_kernel_entry_c` immediately
builds the real, complete kernel page table
([kernel.c](../../kernel/kernel/src/kernel.c)):

```c
static void set_kernel_vm(page_dir_entry_t* vm) {
    memset(vm, 0, PAGE_DIR_SIZE);
    // the kernel image
    map_pages(vm, KERNEL_BASE, _sys_info.phy_offset, V2P(KERNEL_IMAGE_END),
              AP_RW_D, PTE_ATTR_WRBACK_ALLOCATE);
    // the kernel page-directory region
    map_pages(vm, KERNEL_PAGE_DIR_BASE, ...);
    // the kernel heap
    map_pages(vm, KMALLOC_BASE, ...);
    // MMIO (note the permissions: kernel-only, device attributes = no caching)
    map_pages_size(vm, _sys_info.mmio.v_base, _sys_info.mmio.phy_base,
                   _sys_info.mmio.size, AP_RW_D, PTE_ATTR_DEV);
}
```

`map_pages` is literally "writing page tables": allocating/filling L0~L3
entries level by level. A page-table entry (**PTE**, Page Table Entry)
carries attribute bits alongside the physical address:

| Attribute | Effect |
|------|------|
| `AP_RW_D` | readable/writable only by the kernel (privileged); EL0 access faults |
| `AP_RW_RW` | accessible by both kernel and user |
| `PTE_ATTR_WRBACK` | normal memory, caching enabled |
| `PTE_ATTR_DEV` | device memory: no caching, no reordering |

Then `set_translation_table_base(V2P(_kernel_info.kernel_vm))` loads the new
table into `TTBR1_EL1`, and the old table retires with honor.

## 5.7 The Physical Page Allocator: kalloc and the Kernel Heap

With page tables in place, someone must manage "which physical pages are
free". EwokOS has two layers of allocators, each with its own job:

1. **`kalloc`** ([kalloc.c](../../kernel/kernel/src/mm/kalloc.c)):
   allocates in units of 4 KB physical pages, dedicated to allocating memory
   **for the page tables themselves** (because when mapping ordinary memory,
   the ordinary memory manager may not be ready yet);
2. **`kmalloc`** ([kmalloc.c](../../kernel/kernel/src/mm/kmalloc.c)):
   the kernel heap, allocating small objects of any size (structs, strings),
   like userland's `malloc`.

### kalloc's Two-Tier Design: Whole Pages + 1KB Blocks

If the kernel only wants one 128-byte struct, handing over a whole 4 KB page
is wasteful. So besides "whole pages", `kalloc` maintains a **1 KB small
block** mechanism:

```
kalloc
 ├─ whole-page path: _free_list_page — a linked list whose nodes hide at
 │            the start of each free page (the page is idle anyway, so its
 │            first 8 bytes serve as the "next free page" pointer)
 └─ small-block path: split a 4KB page into four 1KB blocks,
              with split_page_meta_t recording "this page was split, which
              blocks are free" (free_mask); small requests come from here,
              and once all 4 blocks return, the whole page is returned
```

**The trick is "metadata reuses the free memory itself"**: the linked-list
nodes of free pages/blocks live directly inside the free memory, occupying
no extra space. A classic move in bare-metal memory managers.

### Page Reference Counting: `_pages_ref`

At the top of kalloc.c sits a global array:

```c
pages_ref_t _pages_ref;   // one counter per physical page
inline uint32_t page_ref_index(ewokos_addr_t paddr) {
    return (paddr - _pages_ref.phy_base) / PAGE_SIZE;
}
```

It records **how many page tables currently reference each physical page**.
Normally it's 1; `fork()`'s copy-on-write bumps it to 2 (parent and child
sharing). This number is the sole evidence for the later "whoever writes
first triggers the copy" verdict — used in Ch. 07.

### kmalloc: the Kernel Heap

`kmalloc` manages the whole virtual range from `KMALLOC_BASE` to
`KMALLOC_END` ([kmalloc.c](../../kernel/kernel/src/mm/kmalloc.c)). It reuses
a generic `malloc_t` allocator framework and only needs to provide 4
callbacks:

```c
_kmalloc.expand   = km_expand;    // when out, push the heap tail forward (not past KMALLOC_END)
_kmalloc.shrink   = km_shrink;    // on shrink, pull the heap tail back
_kmalloc.get_mem_tail = km_get_mem_tail;  // how far the heap is used
_kmalloc.get_mem_top  = km_get_mem_top;   // where the heap's ceiling is
```

In other words, **the kernel heap and userland `malloc` use the same
"linked list + first fit" algorithm** — only the "whom to ask for new
memory" callback differs. Understanding one means understanding both.

### Shared Memory and Page Faults: Two Roads of On-Demand Allocation

`malloc` expansion and fork's copy-on-write both rely on "**use first,
supply later**" lazy allocation:

- **Shared memory (shm)**: [shm.c](../../kernel/kernel/src/mm/shm.c)
  provides `shm_get` (create/find a shared region), `shm_proc_map` (map it
  into a process's page table), and `shm_contig_phy_addr` (query the
  region's physical address — for DMA, see Ch. 13). Two processes mapping
  the same physical page can exchange big data with zero copying;
- **Page faults**: accessing a page whose "virtual address is registered but
  no physical page is assigned yet" makes the MMU throw an exception, and
  the kernel **allocates a page on the spot inside the handler, fills the
  page table, and re-runs that instruction**. The user program notices
  nothing — this is "allocate a page only when it's touched" in action.

The initialization order in `_kernel_entry_c` reflects the layering:

```c
kmalloc_init();              // bring up the minimal kernel heap first, so structs can be created
...
init_allocable_mem();        // then bring all remaining physical memory under management
```

## 5.8 A Process's Private Memory: clone_kernel_vm

Every new process needs its own page table. See
[the kernel's clone_kernel_vm](../../kernel/kernel/src/kernel.c) (AArch64
version):

```c
static void clone_kernel_vm(page_dir_entry_t* vm) {
    uint32_t kernel_l1_base = PAGE_ROOT_INDEX(KERNEL_BASE);
    memset(vm, 0, PAGE_DIR_SIZE);

    // upper half (kernel): copy the kernel page-table pointers verbatim —
    // all processes share the same kernel mappings
    for(uint32_t i = kernel_l1_base; i < PAGE_DIR_NUM; i++)
        vm[i] = _kernel_info.kernel_vm[i];

    // lower half (user): empty, waiting for the process's own
    // code/data/heap/stack to be mapped in
    ...
}
```

So each process's world is:

```
process A's page table            process B's page table
┌──────────────────────┐          ┌──────────────────────┐
│ lower half: A's code/data        │ lower half: B's code/data    ← invisible to each other
├──────────────────────┤          ├──────────────────────┤
│ upper half: kernel (shared) ════ │ upper half: kernel (shared)  ← the same copy
└──────────────────────┘          └──────────────────────┘
```

A process switch only swaps `TTBR0` (the lower-half page-table base); the
kernel part doesn't move a hair.

### At Runtime: How Memory Grows on Demand

A user process's memory is not granted all at once — it is **take as you
use, ask as you need**. Three growth paths:

1. **`malloc` expansion**: when userland `malloc`'s heap runs out, the
   library issues the `SYS_MALLOC_EXPAND` system call (Ch. 08), and the
   kernel **allocates physical pages + writes the page table** in the lower
   half, pushing the heap tail forward;
2. **Lazy allocation on page faults**: regions like the stack and shared
   memory, which "have reserved virtual addresses but no physical pages
   yet", fault on their first real access, and the kernel patches in a page
   on the spot — no need to allocate everything upfront;
3. **fork copy-on-write**: a newborn child's page tables point at the
   parent's physical pages (read-only); whoever writes first gets a **fresh
   copied page** from the kernel, which is then modified. Most children
   `exec` right after `fork`, so that "copy" never happens — precisely the
   key memory saving.

One sentence to sum up this chapter's memory philosophy: **virtual
addresses deliver "looks contiguous, vast, and exclusive"; page tables
deliver "translation"; the physical page allocator delivers "actually
giving"; the reference counters deliver "who must copy"**. Working together,
they produce the illusion that "every process owns all of memory".

## 5.9 Exercises

1. Open [mmu.h](../../kernel/kernel/include/mm/mmu.h), find the definitions
   of `KERNEL_BASE`, `MMIO_BASE`, `KMALLOC_BASE`, and draw your own memory
   layout diagram;
2. After booting, compare the address ranges printed by `show_config()` in
   the boot log against your diagram (which addresses are `kernel image`,
   `kmalloc`, `mmio_base`);
3. Food for thought: if a virtual page in two processes maps to **the same
   physical memory**, what can you do with that? (Answer: shared memory —
   EwokOS's `shm` mechanism, used by the graphics system in Ch. 13.)

## 5.10 Summary

- A virtual address is a translation contract, delivering isolation,
  position independence, and non-contiguous on-demand allocation;
- The MMU translates virtual to physical addresses following the page
  table, which the kernel maintains;
- AArch64 uses 4-level page tables: `TTBR0` for user low addresses,
  `TTBR1` for kernel high addresses;
- The EwokOS kernel sits at `KERNEL_BASE=0x4000000000`, with MMIO right
  behind;
- At boot, enable the MMU with a minimal table first, then build the real
  kernel page table;
- `kalloc` manages physical pages (two tiers: whole pages + 1KB blocks);
  `kmalloc` manages the kernel heap;
- `_pages_ref` reference counting backs copy-on-write; `shm` and page
  faults are the two roads of "on-demand allocation";
- Every process shares the kernel's upper-half page tables and owns a
  private lower half — isolation and efficiency at once.

Next chapter: interrupts and timers — giving the kernel a sense of "time"
and the ability to respond to external events promptly.
