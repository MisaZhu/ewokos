#ifndef MMU_H
#define MMU_H

#include <mm/mmu_arch.h>
#include <kernel/hw_info.h>
#include <ewokos_config.h>
#include <syscalls.h>   /* EWOK_MMAP_ANON_ALIGN, EWOK_PROT_* */


#define ALIGN_DOWN(x, alignment) ((ewokos_addr_t)(x) & ~((ewokos_addr_t)(alignment) - 1))
#define ALIGN_UP(x, alignment) (((ewokos_addr_t)(x) + (ewokos_addr_t)(alignment) - 1) & ~((ewokos_addr_t)(alignment) - 1))

#define V2P(V) ((ewokos_addr_t)(V) - KERNEL_BASE + _sys_info.phy_offset)
#define P2V(P) ((ewokos_addr_t)(P) - _sys_info.phy_offset + KERNEL_BASE)

#define KERNEL_IMAGE_END              ALIGN_UP((ewokos_addr_t)_kernel_end, PAGE_SIZE)

#define KERNEL_VSYSCALL_INFO_BASE     KERNEL_IMAGE_END
#define KERNEL_VSYSCALL_INFO_SIZE     (ALIGN_UP(sizeof(vsyscall_info_t), PAGE_SIZE))
#define KERNEL_VSYSCALL_INFO_END      (KERNEL_VSYSCALL_INFO_BASE+KERNEL_VSYSCALL_INFO_SIZE)

#define KERNEL_PAGE_DIR_BASE          ALIGN_UP(KERNEL_VSYSCALL_INFO_END, PAGE_DIR_SIZE)
#if __aarch64__
#define KERNEL_PAGE_DIR_SIZE          (1*MB)
#else
#define KERNEL_PAGE_DIR_SIZE          (256*KB)
#endif
#define KERNEL_PAGE_DIR_END           (KERNEL_PAGE_DIR_BASE + KERNEL_PAGE_DIR_SIZE)

#define ALLOCABLE_PAGE_DIR_BASE       ALIGN_UP(KERNEL_PAGE_DIR_END, PAGE_DIR_SIZE)
#if __aarch64__
#define ALLOCABLE_L3_PAGE_TABLE_SIZE  \
	((ALIGN_UP(_sys_info.total_usable_mem_size, PAGE_TABLE_SPAN_L3) / PAGE_TABLE_SPAN_L3) * PAGE_TABLE_SIZE)
/* Low identity mappings are also built from this pool after early boot. */
#define ALLOCABLE_PAGE_DIR_EXTRA_SIZE (1*MB)
#ifdef PAGE_SIZE_64K
#define ALLOCABLE_PAGE_DIR_SIZE       \
	(ALLOCABLE_L3_PAGE_TABLE_SIZE + ALLOCABLE_PAGE_DIR_EXTRA_SIZE)
#else
#define ALLOCABLE_L2_PAGE_TABLE_SIZE  \
	((ALIGN_UP(_sys_info.total_usable_mem_size, PAGE_TABLE_SPAN_L2) / PAGE_TABLE_SPAN_L2) * PAGE_TABLE_SIZE)
#define ALLOCABLE_PAGE_DIR_SIZE       \
	(ALLOCABLE_L3_PAGE_TABLE_SIZE + ALLOCABLE_L2_PAGE_TABLE_SIZE + \
	 ALLOCABLE_PAGE_DIR_EXTRA_SIZE)
#endif
#else
#define ALLOCABLE_PAGE_DIR_SIZE       (2 * (_sys_info.total_phy_mem_size / KB))
#endif
#define ALLOCABLE_PAGE_DIR_END        (ALLOCABLE_PAGE_DIR_BASE + ALLOCABLE_PAGE_DIR_SIZE)

#define KMALLOC_BASE                  ALLOCABLE_PAGE_DIR_END
#define KMALLOC_END                   (KMALLOC_BASE + _sys_info.kmalloc_size)

/* attribute shared by every mapping of the sys_dma window (kernel identity,
   per-process identity, dma_alloc, peer map). Machines whose DMA masters are
   coherent (e.g. QEMU virt, where the "device" is the emulator's cacheable
   host mapping) may override this to a cacheable attribute: a Non-cacheable
   guest alias cannot observe writes the emulator leaves in host CPU caches,
   which breaks virtqueue completion polling under KVM. */
#ifndef PTE_ATTR_SYS_DMA
#define PTE_ATTR_SYS_DMA              PTE_ATTR_NOCACHE
#endif

#ifndef MAX_USABLE_MEM_SIZE
#ifdef __aarch64__
#define MAX_USABLE_MEM_SIZE           (32ull*GB) //max usable memory for 64bits OS
#else
#define MAX_USABLE_MEM_SIZE           (1*GB + 640*MB) //max usable memory for 32bits OS
#endif
#endif

#define MMIO_MAX_SIZE                 (128*MB)
#define MMIO_BASE                     (KERNEL_BASE + MAX_USABLE_MEM_SIZE)
#define MMIO_END                      (MMIO_BASE + MMIO_MAX_SIZE)

#define DMA_V_BASE                    (MMIO_END)
#define DMA_END                       (DMA_V_BASE + _sys_info.sys_dma.size)

#define USER_STACK_TOP                (KERNEL_BASE - PAGE_SIZE)
#define USER_STACK_MAX                (64*MB)
#define USER_STACK_BOTTOM             (USER_STACK_TOP - USER_STACK_MAX)

/* Default maximum reachability range shared by sys_dma and shm physical
   allocation. A pure address cap, NOT a reservation: the pages stay general
   allocable RAM and are claimed on demand. The per-process whole-range DMA
   identity pre-map was removed (sys_dma_alloc maps each buffer on demand into
   the high DMA_V_BASE window), so the span no longer costs per-fork page
   tables and a generous 1G default is safe. Small boards are clamped to their
   arch_dma_range() hardware limit (publish_sys_dma_range only shrinks). */
#define DMA_SHM_RANGE_DEFAULT         (1*GB)

/* Shm virtual window. EVERY segment is backed by a dynamically claimed
   physically-contiguous run of general RAM (the dma_alloc mechanism) and
   mapped NOCACHE, so any of them can serve as a g2d/DMA canvas. Boards
   without a g2d engine (QCOM8GEN3/GOP) put every canvas here too: the
   display keeps two full frames (scan-out shm + libdisplay's cache_g back
   buffer) and every window keeps ws_g, ws_g2 (fps_async), frame_g and often a
   frosted backdrop. A 3840x2160 canvas is 31.6MB, so 4K needs ~66MB just for
   the display and ~130MB for one full-screen window - the old 128MB window
   overflowed and graph_new_shm() returned NULL, leaving xwin_open() to fail
   while the app kept running with no window. Real 1080p sessions measure
   ~57MB of shm, i.e. ~230MB scaled to 4K. Both the virtual window and the
   physical allocation cap default to DMA_SHM_RANGE_DEFAULT (1G): on aarch64
   the user half is 256GB so this is trivial; on 32-bit (2GB user half) the
   top-down stack/shm/kmalloc-vm/dl-window chain still leaves ~640MB below
   DL_WINDOW_BASE for heap/load, ample for every 32-bit board's physical RAM. */
#define SHM_MAX_SIZE                  DMA_SHM_RANGE_DEFAULT
#define SHM_BASE                      (USER_STACK_BOTTOM - SHM_MAX_SIZE)

#define KMALLOC_VM_SIZE               (64*MB)
#define KMALLOC_VM_BASE               (SHM_BASE - KMALLOC_VM_SIZE)

/* Anonymous user-mapping window: the backing store for SYS_MMAP_ANON and where
   a dynamic loader's objects live. Enabled on EVERY architecture - it is the
   next free slot in the chain above, [DL_WINDOW_BASE, KMALLOC_VM_BASE), left
   unmapped in each process until the window commits a slot.

   Why not the heap: heap pages are uniformly AP_RW_RW and, worse, a write fault
   inside [rw_heap_base, heap_size) or the user stack is silently repaired by
   copy_on_write() (irq.c), which remaps the page writable - so read-only text
   cannot be enforced there at all. The window sits outside that range, so its
   per-page permissions are real: a loader marks text AP_RW_R (read-only) and
   data AP_RW_RW, and a stray store into text is a genuine access-denied fault.

   Per-page R vs R+W is honored by map_page on every arch (x86 rw bit, arm ap
   field, riscv permission bits), so read-only text works everywhere. The
   execute-never half of W^X is best-effort: aarch64 marks UXN/PXN through its
   arch_mark_pte_noexec, while other arches leave that hook as the weak no-op,
   so their data pages may remain executable. The top-down stack/shm/kmalloc-vm/
   dl-window chain is sized to fit inside the 2GB user VA on 32-bit arches
   (see SHM_MAX_SIZE above). */
#define DL_WINDOW_SIZE                (256*MB)
#define DL_WINDOW_BASE                (KMALLOC_VM_BASE - DL_WINDOW_SIZE)
#define DL_WINDOW_END                 (KMALLOC_VM_BASE)
/* Allocation granularity. Fixed by the user-visible ABI, never by PAGE_SIZE,
   so libc and the kernel agree without libc knowing the page size; it is a
   multiple of every PAGE_SIZE EwokOS builds for (4k/16k/64k). */
#define DL_WINDOW_SLOT                EWOK_MMAP_ANON_ALIGN
#define DL_WINDOW_SLOTS               (DL_WINDOW_SIZE / DL_WINDOW_SLOT)
#define DL_WINDOW_PAGES_PER_SLOT      (DL_WINDOW_SLOT / PAGE_SIZE)

#define get64(addr) (*((volatile uint64_t *)(addr)))
#define put64(addr, val) (*((volatile uint64_t *)(addr)) = (uint64_t)(val))
#define get32(addr) (*((volatile uint32_t *)(addr)))
#define put32(addr, val) (*((volatile uint32_t *)(addr)) = (uint32_t)(val))
#define get16(addr) (*((volatile uint16_t *)(addr)))
#define put16(addr, val) (*((volatile uint16_t *)(addr)) = (uint16_t)(val))
#define get8(addr) (*((volatile uint8_t *)(addr)))
#define put8(addr, val) (*((volatile uint8_t *)(addr)) = (uint8_t)(val))

int32_t map_pages(page_dir_entry_t *vm, ewokos_addr_t vaddr,
	ewokos_addr_t pstart,
	ewokos_addr_t pend,
	uint32_t access_permissions,
	uint32_t pte_attr);

int32_t map_pages_size(page_dir_entry_t *vm, ewokos_addr_t vaddr,
	ewokos_addr_t pstart,
	uint32_t size,
	uint32_t access_permissions,
	uint32_t pte_attr);

void unmap_pages(page_dir_entry_t *vm, ewokos_addr_t virtual_addr, uint32_t pages);

int32_t map_page_ref(page_dir_entry_t *vm, ewokos_addr_t vaddr, ewokos_addr_t paddr, uint32_t permissions, uint32_t pte_attr);
void unmap_page_ref(page_dir_entry_t *vm, ewokos_addr_t vaddr);

/*
 * Physical page migration. Arch hook: walk every leaf PTE of vm, count the
 * ones mapping phy_from and, when apply != 0, retarget them to phy_to with
 * the virtual address and all attribute bits unchanged. Returns the count,
 * or -1 when a mapping cannot be retargeted page-exactly (block descriptor)
 * or the arch has no implementation (weak default in mm/mmu.c).
 */
int32_t arch_retarget_ptes(page_dir_entry_t *vm, ewokos_addr_t phy_from,
	ewokos_addr_t phy_to, int apply);
/*
 * Revoke user mappings of a physical run. Arch hook: walk the leaf PTEs of vm
 * and invalidate (EntryType = 0) every one whose output address falls inside
 * [phy_base, phy_base + phy_size), leaving identity / direct-map aliases alone.
 * Returns the number of invalidated PTEs, or -1 when a block descriptor covers
 * part of the run (cannot be revoked page-exactly) or the arch has no
 * implementation (weak no-op default in mm/mmu.c returns 0). The caller owes a
 * flush_tlb() before the pages are recycled. Used by SYS_FREE_PHY_CONTIG to
 * drop a caller's SYS_MEM_MAP_NOCACHE mapping before its run returns to the allocator.
 */
int32_t arch_unmap_ptes_phy(page_dir_entry_t *vm, ewokos_addr_t phy_base,
	ewokos_addr_t phy_size);
/* Move page content phy_from -> phy_to and retarget every PTE of every
 * address space accordingly; phy_from is returned to the page allocator.
 * Returns the number of retargeted PTEs, or -1 (nothing changed). */
int32_t page_move(ewokos_addr_t phy_from, ewokos_addr_t phy_to);

/*
 * Allocate size bytes of physically-contiguous memory inside the physical
 * range [phy_base, phy_base + phy_size); phy_size == 0 means no range limit.
 * size is rounded up to PAGE_SIZE, phy_base up to a page boundary, and the
 * returned physical address is page-aligned. When no free run exists, movable
 * user pages inside the range are compacted via page_move (same contract as
 * dma_alloc: caller holds the kernel lock with pages quiescent). Returns the
 * physical base of the run, or 0 on failure. Release with kfree_contig().
 */
ewokos_addr_t mm_alloc_contig(uint32_t size, ewokos_addr_t phy_base,
	ewokos_addr_t phy_size);

/* Rewrite the memory attribute of the kernel direct-map alias P2V(paddr) of
 * one physical page (address/permissions preserved). Arch hook: aarch64
 * overrides it; the weak default is a no-op for arches whose DMA attribute
 * equals WRBACK. Caller owes dcache maintenance + flush_tlb around the flip. */
void arch_set_direct_map_attr(ewokos_addr_t paddr, uint32_t pte_attr);

void free_page_tables(page_dir_entry_t *vm);
ewokos_addr_t resolve_kernel_address(page_dir_entry_t *vm, ewokos_addr_t virtual);

#endif
