#ifndef SYS_H
#define SYS_H

#include <sysinfo.h>

#ifdef __cplusplus
extern "C" {
#endif

int sys_get_sys_info(sys_info_t* info);
extern vsyscall_info_t* _vsyscall_info;

/*
 * Publish freshly written code at [vaddr, vaddr+size) to the instruction
 * fetch path. EL0 cannot run DC CIVAC / IC IVAU itself on EwokOS (the kernel
 * leaves SCTLR_EL1.UCI clear), so a dynamic loader or a JIT must ask the
 * kernel to clean the D-cache to PoU and broadcast an I-cache invalidate.
 * Returns 0 on success, -1 when the range is rejected or nothing is current.
 */
int sys_cache_sync(void* vaddr, unsigned int size);

/*
 * Anonymous per-page-permission mappings drawn from the kernel's dedicated
 * user-mapping window (SYS_MMAP_ANON / SYS_MPROTECT / SYS_MUNMAP; see
 * DL_WINDOW_* in the kernel's <mm/mmu.h>). These exist so a dynamic loader can
 * give a plugin's text segment read-only-and-execute and its data segment
 * read-write (W^X); the sbrk heap cannot express per-page permissions, so
 * libc's mmap()/mprotect() prefer these and fall back to heap emulation when
 * sys_mmap_anon() reports no window.
 *
 * sys_mmap_anon returns NULL when the kernel has no window (a 32-bit target,
 * where DL_WINDOW_SIZE is 0, or a kernel older than these calls) or the window
 * is full - in both cases the caller falls back to the heap emulation, so NULL
 * is never a hard error. `size` is rounded up to EWOK_MMAP_ANON_ALIGN inside
 * both sys_mmap_anon and sys_munmap so the two always release exactly what was
 * allocated. `prot` is an EWOK_PROT_* word, equal to <sys/mman.h>'s PROT_*.
 * sys_mprotect and sys_munmap return 0 on success, -1 on a rejected range.
 */
void* sys_mmap_anon(unsigned int size, int prot);
int   sys_mprotect(void* vaddr, unsigned int size, int prot);
int   sys_munmap(void* vaddr, unsigned int size);

#ifdef __cplusplus
}
#endif

#endif
