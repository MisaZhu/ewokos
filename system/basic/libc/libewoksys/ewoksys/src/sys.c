#include <ewoksys/sys.h>
#include <ewoksys/syscall.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

int sys_get_sys_info(sys_info_t* info) {
    if(info == NULL)
        return -1;

    memset(info, 0, sizeof(sys_info_t));
    int ret = syscall2(SYS_GET_SYS_INFO, (ewokos_addr_t)info,
            (ewokos_addr_t)sizeof(sys_info_t));
    if(ret < 0)
        return ret;

    info->machine[MACHINE_MAX-1] = 0;
    info->arch[ARCH_MAX-1] = 0;
    if(info->cores > MAX_CORE_NUM)
        info->cores = MAX_CORE_NUM;
    return 0;
}

int sys_cache_sync(void* vaddr, unsigned int size) {
    if(vaddr == NULL || size == 0)
        return -1;
    return (int)syscall2(SYS_CACHE_SYNC, (ewokos_addr_t)vaddr, (ewokos_addr_t)size);
}

/* Round up to the window's allocation granularity so an alloc and the matching
   free cover exactly the same slots. The kernel rounds its own allocation up to
   EWOK_MMAP_ANON_ALIGN but returns only the address, so the free must repeat
   the rounding or it would leave the tail of the last slot mapped and its
   physical pages unreclaimed. */
static unsigned int mmap_anon_round(unsigned int size) {
    unsigned int a = (unsigned int)EWOK_MMAP_ANON_ALIGN;
    if(a == 0)
        return size;
    return (unsigned int)(((ewokos_addr_t)size + (a - 1)) & ~(ewokos_addr_t)(a - 1));
}

void* sys_mmap_anon(unsigned int size, int prot) {
    ewokos_addr_t r;
    if(size == 0)
        return NULL;
    /* The kernel returns 0 for "no window here" (a 32-bit target, an older
       kernel) as well as for a genuine out-of-window failure; 0 can never be a
       window address, so NULL is handed back for the caller to fall back on. */
    r = syscall2(SYS_MMAP_ANON, (ewokos_addr_t)mmap_anon_round(size), (ewokos_addr_t)prot);
    return (void*)r;
}

int sys_mprotect(void* vaddr, unsigned int size, int prot) {
    if(vaddr == NULL || size == 0)
        return -1;
    return (int)syscall3(SYS_MPROTECT, (ewokos_addr_t)vaddr, (ewokos_addr_t)size,
            (ewokos_addr_t)prot);
}

int sys_munmap(void* vaddr, unsigned int size) {
    if(vaddr == NULL || size == 0)
        return -1;
    return (int)syscall2(SYS_MUNMAP, (ewokos_addr_t)vaddr,
            (ewokos_addr_t)mmap_anon_round(size));
}

#ifdef __cplusplus
}
#endif
