#ifndef SYSCALLS_H
#define SYSCALLS_H


#define IPC_ERROR_RETRY      -1
#define IPC_ERROR_SELF       -2
#define IPC_ERROR_NO_READY   -3
#define IPC_ERROR_IN_INTR    -3

#define DMA_MAGIC   0xCAFECAFF

/* Architecture-specific, privileged controls. Outside the counted enum to
 * preserve sys_state_t's ABI for existing rootfs binaries. */
#define SYS_PLATFORM_CTL 0x100

/* SYS_SEMAPHORE_ALLOC: arg0=0 allocates a private semaphore (legacy ABI).
 * Nonzero keys retrieve kernel-owned hardware locks. Successful lookups
 * return SYS_SEM_HANDLE_TAG | id; old kernels return an untagged private id,
 * which callers must free and reject rather than running without a lock. */
#define SYS_SEM_VC_MAILBOX 1u
#define SYS_SEM_COUNT      1u
#define SYS_SEM_HANDLE_TAG 0x40000000u

/*
 * ABI of the anonymous user-mapping window (SYS_MMAP_ANON / SYS_MPROTECT /
 * SYS_MUNMAP below, DL_WINDOW_* in <mm/mmu.h> on the kernel side).
 *
 * EWOK_MMAP_ANON_ALIGN is both the allocation granularity and the alignment
 * the kernel guarantees for an address it returns. libc rounds a requested
 * length UP to it before calling and records that rounded length, so the
 * matching SYS_MUNMAP always releases whole slots; a caller that unmapped a
 * partial slot would leave the rest of it unallocatable.
 *
 * EWOK_PROT_* must stay equal to <sys/mman.h>'s PROT_*, which lets libc hand
 * a prot word straight through untranslated. libgloss checks the equality at
 * build time; if newlib ever renumbers them, that check fails the build
 * instead of silently granting the wrong permission.
 */
#define EWOK_MMAP_ANON_ALIGN (64*1024)
#define EWOK_PROT_NONE  0x0
#define EWOK_PROT_READ  0x1
#define EWOK_PROT_WRITE 0x2
#define EWOK_PROT_EXEC  0x4

enum {
	SYS_NONE = 0,
	SYS_KPRINT, 
	//proccess memory manage
	SYS_MALLOC_EXPAND,
	SYS_MALLOC_SIZE,
	SYS_FREE,

	SYS_EXEC_ELF,
	SYS_FORK,
	SYS_THREAD,
	SYS_YIELD,
	SYS_WAIT_PID,
	SYS_USLEEP,
	SYS_EXIT,
	SYS_DETACH,
	SYS_BLOCK,
	SYS_WAKEUP,
	SYS_SIGNAL_SETUP,
	SYS_SIGNAL,
	SYS_SIGNAL_END,

	SYS_GET_PID,
	SYS_GET_THREAD_ID,

	SYS_IPC_PING,
	SYS_IPC_READY,

	SYS_PROC_GET_CMD,
	SYS_PROC_GET_UID,
	SYS_PROC_SET_UID,

	//share memory syscalls
	SYS_PROC_SHM_GET,
	SYS_PROC_SHM_MAP,
	SYS_PROC_SHM_UNMAP,

	SYS_GET_SYS_INFO,
	SYS_GET_SYS_STATE,
	SYS_GET_VSYSCALL_INFO,
	SYS_GET_PROC,
	SYS_GET_PROCS_NUM,
	SYS_GET_PROCS,

	//map mmio memory for userspace access
	SYS_MEM_MAP,
	SYS_V2P,
	SYS_P2V,

	//internal proccess communication
	SYS_IPC_SETUP,
	SYS_IPC_CALL,
	SYS_IPC_GET_ARG,
	SYS_IPC_SET_RETURN,
	SYS_IPC_GET_RETURN,
	SYS_IPC_END,
	SYS_IPC_DISABLE,
	SYS_IPC_ENABLE,

	SYS_CORE_READY,
	SYS_CORE_PID,
	SYS_GET_KEVENT,

	SYS_INTR_SETUP,
	SYS_INTR_END,

	SYS_SEMAPHORE_ALLOC,
	SYS_SEMAPHORE_FREE,
	SYS_SEMAPHORE_ENTER,
	SYS_SEMAPHORE_TRY_ENTER,
	SYS_SEMAPHORE_QUIT,
	SYS_SOFT_INT,

	SYS_PROC_GET_GID,
	SYS_PROC_SET_GID,

	SYS_DMA_ALLOC,
	SYS_DMA_FREE,
	SYS_DMA_PHY_ADDR,
	SYS_DMA_SET,

	SYS_MMIO_RW,
	SYS_PROC_PRIORITY,

	SYS_SHM_CONTIG_PHY_ADDR,
	SYS_PROC_SHM_CTRL,
	SYS_BLOCK_TIMEOUT,
	SYS_SEMAPHORE_ENTER_TIMEOUT,
	SYS_SEMAPHORE_ALLOC_COUNT,
	SYS_SEMAPHORE_GET_COUNT,
	/* drop a cross-proc dma mapping installed by SYS_MEM_MAP on a sys_dma
	   range (peer-side voluntary unmap; see dma_peer_unmap) */
	SYS_DMA_UNMAP,

	//capability management (see cap.h)
	SYS_CAP_MINT,
	SYS_CAP_REVOKE,
	SYS_CAP_GRANT,
	SYS_CAP_GET,
	/* install one entry into the kernel's exec-time capability policy
	   (CAP_ROOT only); matching rules are granted automatically on exec */
	SYS_CAP_POLICY_ADD,

	/* Publish user-written bytes to the instruction-fetch path:
	   arg0 = user vaddr, arg1 = size. Cleans the D-cache to PoU over the
	   range and broadcasts an I-cache invalidate. Needed because EL0 cannot
	   run DC CIVAC / IC IVAU itself (SCTLR_EL1.UCI is left clear), so a
	   dynamic loader or a JIT has no other way to make new code executable. */
	SYS_CACHE_SYNC,

	/* Anonymous user mappings drawn from the per-process window described by
	   DL_WINDOW_* in <mm/mmu.h> instead of from the sbrk heap. A dynamic
	   loader needs per-page permissions to give a plugin's text segment RO+X
	   and its data segment RW; the heap cannot express that (every heap page
	   is RW+X and mprotect() over it was a no-op), so plugin code used to sit
	   in the same pages as malloc's free lists.

	   SYS_MMAP_ANON(size, prot) returns a window address, or 0 when the
	   window is absent or exhausted - libc reads 0 as "fall back to the heap
	   emulation", so mmap() keeps working on targets without a window.
	   SYS_MPROTECT(vaddr, size, prot) and SYS_MUNMAP(vaddr, size) only act
	   inside the window and reject anything else. */
	SYS_MMAP_ANON,
	SYS_MPROTECT,
	SYS_MUNMAP,
	SYS_CALL_NUM
};

#endif
