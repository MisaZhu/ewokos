#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ewoksys/syscall.h>
#include <ewoksys/sys.h>
#include <sysinfo.h>

static inline uint32_t addr_hi(ewokos_addr_t v) {
    return (uint32_t)(v >> 32);
}

static inline uint32_t addr_lo(ewokos_addr_t v) {
    return (uint32_t)(v & 0xffffffffu);
}

/* one-decimal size description: get_mem_size_desc() rounds whole MB up,
   which hides small DMA/shm occupancy; keep the fraction here instead */
static const char* mem_desc(ewokos_addr_t size, char* ret, uint32_t len) {
    if(size >= 1024ULL*1024*1024)
        snprintf(ret, len, "%llu.%lluG",
                (unsigned long long)(size >> 30),
                (unsigned long long)(((size & 0x3fffffffULL) * 10) >> 30));
    else if(size >= 1024ULL*1024)
        snprintf(ret, len, "%llu.%lluM",
                (unsigned long long)(size >> 20),
                (unsigned long long)(((size & 0xfffffULL) * 10) >> 20));
    else if(size >= 1024)
        snprintf(ret, len, "%lluK", (unsigned long long)(size >> 10));
    else
        snprintf(ret, len, "%lluB", (unsigned long long)size);
    return ret;
}

static inline uint32_t percent(ewokos_addr_t used, ewokos_addr_t total) {
    /* 64-bit intermediate: on 32-bit targets used*100 overflows above ~42MB */
    return (total > 0) ? (uint32_t)((uint64_t)used * 100 / total) : 0;
}

int main(int argc, char* argv[]) {
    sys_info_t sys_info;
    sys_state_t sys_state;

    sys_get_sys_info(&sys_info);
    syscall1(SYS_GET_SYS_STATE, (ewokos_addr_t)&sys_state);

    /* Exact accounting over the usable RAM span:
       usable = kernel core (image + page-table pools + kmalloc capacity)
              + allocable span + top reserved hole.
       The sys_dma range is NOT a carve-out: its pages live in the
       allocable span, and shm segments draw contiguous runs from the same
       general RAM, so dma_used / shm bytes are a breakdown OF the
       allocable usage, never added on top of it.
       kmalloc free (trunk_free_size) already includes the not-yet-committed
       tail up to KMALLOC_END, so kmalloc used = capacity - kfree exactly. */
    ewokos_addr_t kernel_span = sys_info.allocable_phy_mem_base - sys_info.phy_offset;
    ewokos_addr_t allocable_span = sys_info.allocable_phy_mem_top - sys_info.allocable_phy_mem_base;
    ewokos_addr_t reserved = sys_info.phy_offset + sys_info.total_usable_mem_size -
        sys_info.allocable_phy_mem_top;
    ewokos_addr_t kmalloc_cap = sys_info.kmalloc_size;
    ewokos_addr_t kmalloc_used = (kmalloc_cap > sys_state.mem.kfree) ?
        (kmalloc_cap - sys_state.mem.kfree) : 0;
    /* free 1K chunks sit in split pages that left the whole-page free list,
       so they must be added back for the exact free capacity */
    ewokos_addr_t free_allocable = sys_state.mem.free + sys_state.free_1k;
    ewokos_addr_t used_allocable = (allocable_span > free_allocable) ?
        (allocable_span - free_allocable) : 0;
    ewokos_addr_t free_total = free_allocable + sys_state.mem.kfree;
    ewokos_addr_t used_total = (sys_info.total_usable_mem_size > free_total) ?
        (sys_info.total_usable_mem_size - free_total) : 0;
    ewokos_addr_t shm_total = sys_state.mem.shared;

    char s[16][32];
    mem_desc(sys_info.total_phy_mem_size, s[0], 31);
    mem_desc(sys_info.total_usable_mem_size, s[1], 31);
    mem_desc(used_total, s[2], 31);
    mem_desc(kernel_span, s[3], 31);
    mem_desc(kmalloc_used, s[4], 31);
    mem_desc(kmalloc_cap, s[5], 31);
    mem_desc(sys_state.mem.kfree, s[6], 31);
    mem_desc(sys_info.sys_dma.size, s[7], 31);
    mem_desc(sys_state.dma_used, s[8], 31);
    mem_desc(shm_total, s[9], 31);
    mem_desc(reserved, s[10], 31);
    mem_desc(free_allocable, s[11], 31);
    mem_desc(allocable_span, s[12], 31);
    char used_alloc[32], mmio[32];
    mem_desc(used_allocable, used_alloc, 31);
    mem_desc(sys_info.mmio.size, mmio, 31);

    printf(
        "machine            %s\n"
        "arch               %s-%dk\n"
        "cores              %u\n"
        "phy mem size       %s\n"
        "usable mem size    %s\n"
        "mem used           %s/%s (%u%%)\n"
        "kernel mem         phy:0x%08x%08x ~ 0x%08x%08x (%s)\n"
        "kmalloc            used %s/%s (free %s)\n"
        "sys_dma            phy:0x%08x%08x,V:0x%08x%08x range %s, used %s (%u%%)\n"
        "shm used           %s\n"
        "reserved mem       phy:0x%08x%08x ~ 0x%08x%08x (%s)\n"
        "allocable mem      phy:0x%08x%08x ~ 0x%08x%08x (free %s/%s, used %s)\n"
        "mmio               phy:0x%08x%08x,V:0x%08x%08x (%s)\n",
        sys_info.machine,
        sys_info.arch, sys_info.page_size/1024,
        sys_info.cores,
        s[0],
        s[1],
        s[2], s[1], percent(used_total, sys_info.total_usable_mem_size),
        addr_hi(sys_info.phy_offset), addr_lo(sys_info.phy_offset),
        addr_hi(sys_info.allocable_phy_mem_base), addr_lo(sys_info.allocable_phy_mem_base),
        s[3],
        s[4], s[5], s[6],
        addr_hi(sys_info.sys_dma.phy_base), addr_lo(sys_info.sys_dma.phy_base),
        addr_hi(sys_info.sys_dma.v_base), addr_lo(sys_info.sys_dma.v_base),
        s[7], s[8], percent(sys_state.dma_used, sys_info.sys_dma.size),
        s[9],
        addr_hi(sys_info.allocable_phy_mem_top), addr_lo(sys_info.allocable_phy_mem_top),
        addr_hi(sys_info.phy_offset + sys_info.total_usable_mem_size),
        addr_lo(sys_info.phy_offset + sys_info.total_usable_mem_size),
        s[10],
        addr_hi(sys_info.allocable_phy_mem_base), addr_lo(sys_info.allocable_phy_mem_base),
        addr_hi(sys_info.allocable_phy_mem_top), addr_lo(sys_info.allocable_phy_mem_top),
        s[11], s[12], used_alloc,
        addr_hi(sys_info.mmio.phy_base), addr_lo(sys_info.mmio.phy_base),
        addr_hi(sys_info.mmio.v_base), addr_lo(sys_info.mmio.v_base),
        mmio);

    return 0;
}