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
   per-process identity, dma_alloc, peer map). */
#define PTE_ATTR_SYS_DMA              PTE_ATTR_NOCACHE

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

/* Scattered (kalloc-backed) shm virtual window. ONLY non-contig segments draw
   addresses from here, so it is fully independent of the contig slab window
   below. Boards without a g2d engine (Q8B/GOP) put every canvas here: the
   display keeps two full frames (scan-out shm + libdisplay's cache_g back
   buffer) and every window keeps ws_g, ws_g2 (fps_async), frame_g and often a
   frosted backdrop. A 3840x2160 canvas is 31.6MB, so 4K needs ~66MB just for
   the display and ~130MB for one full-screen window - the old 128MB window
   overflowed and graph_new_shm() returned NULL, leaving xwin_open() to fail
   while the app kept running with no window. Real 1080p sessions measure
   ~57MB of scattered shm, i.e. ~230MB scaled to 4K, so 64-bit kernels get
   512MB (~2x the 4K baseline); 32-bit ones keep 128MB. */
#ifdef __aarch64__
#define SHM_MAX_SIZE                  (512*MB)
#else
#define SHM_MAX_SIZE                  (128*MB)
#endif
#define SHM_BASE                      (USER_STACK_BOTTOM - SHM_MAX_SIZE)

/* Contiguous shm gets its OWN virtual window, separate from the scattered one,
   so kernel.conf shm_contig_size is actually usable. Every IPC_CONTIG segment
   maps a run of the reserved physical slab [shm_contig.phy_base, phy_base+size)
   into this window, and the runtime bound in shm_alloc is exactly that slab
   size - contig capacity is therefore governed by shm_contig_size alone, never
   by SHM_MAX_SIZE. (Before this split both drew from one 128MB window, so at
   1920x1080 a few full-screen canvases exhausted it and shmget failed even with
   a 256MB slab reserved; clients then tore down xinfo and crashed reading it in
   xwin_repaint.) SHM_CONTIG_VA_MAX is the STATIC reservation that anchors
   KMALLOC_VM below the largest slab a board may configure; sys_info_config caps
   shm_contig.size to it so the physical carve and this window always agree. */
#define SHM_CONTIG_VA_MAX             (512*MB)
#define SHM_CONTIG_BASE               (SHM_BASE - SHM_CONTIG_VA_MAX)

#define KMALLOC_VM_SIZE               (64*MB)
#define KMALLOC_VM_BASE               (SHM_CONTIG_BASE - KMALLOC_VM_SIZE)

/* Anonymous user-mapping window, the backing store for SYS_MMAP_ANON and the
   place a dynamic loader's objects live. It is the next free slot in the
   chain above: KMALLOC_VM ends at KMALLOC_VM_BASE, so
   [DL_WINDOW_BASE, KMALLOC_VM_BASE) is unmapped in every process, because
   clone_kernel_vm shares only the kernel upper half and each space maps the
   user half itself.

   Why not the heap: a dlopen()'d object's text segment has to end up
   read-only-and-executable so a stray store cannot rewrite instructions that
   are about to be fetched, while its data segment stays writable. Every heap
   page is RW+X and mprotect() over it was a no-op, so plugin code shared
   pages with malloc's free lists - one heap overflow away from being
   arbitrary code. The window has real per-page permissions, and because the
   fault handler's copy_on_write() only recognises [rw_heap_base, heap_size)
   and the user stack, a write to a window page the loader made read-only is a
   genuine access-denied fault rather than a silently repaired one.

   AArch64 only. The window is gated on __aarch64__, not on address width: on
   a 32-bit target KERNEL_BASE sits low enough that the chain above already
   claims most of the user VA, and no non-AArch64 target wires up the W^X
   window. DL_WINDOW_SIZE 0 there leaves the window empty so the three syscalls
   fail cleanly and libc falls back to the RWX heap emulation it has always
   used. dlopen() still loads and runs a plugin that way - the x86 dltest suite
   passes - it just gets no per-page W^X. Nothing else in the tree has to
   know. */
#ifdef __aarch64__
#define DL_WINDOW_SIZE                (256*MB)
#else
#define DL_WINDOW_SIZE                0
#endif
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

void free_page_tables(page_dir_entry_t *vm);
ewokos_addr_t resolve_kernel_address(page_dir_entry_t *vm, ewokos_addr_t virtual);

#endif
