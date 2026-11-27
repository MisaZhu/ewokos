#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ewoksys/dma.h>
#include <ewoksys/sys.h>

/*
 * Big-block DMA test for the dynamic sys_dma architecture: one dma_alloc()
 * from block 0 must return a single physically-contiguous run inside the
 * DMA-reachable range (kernel draws it from general RAM via
 * kalloc_contig_compact, not from a fixed carve-out). Verify the physical
 * placement, write a position-dependent pattern over every word, read it
 * all back, then free and re-allocate to prove the run went back to the
 * allocator.
 *
 * Usage: dmatest [MB]  - request size in MB (1..4095, the uint32_t syscall
 * cap); no argument defaults to 128MB.
 */

#define GB (1024ull * 1024ull * 1024ull)

static uint32_t pattern(uint32_t i) {
    return i * 2654435761u + 0xA5A5A5A5u;
}

int main(int argc, char* argv[]) {
    setbuf(stdout, NULL);

    sys_info_t info;
    if(sys_get_sys_info(&info) != 0) {
        printf("dmatest: FAIL sys_get_sys_info\n");
        return 1;
    }

    /* optional argv[1] = request size in MB; default 128MB. Rounded down to
       whole kernel pages (4K/16K/64K builds) so the request matches the
       grant exactly. 4095MB is the uint32_t syscall size cap. */
    uint64_t bytes = 128ull * 1024 * 1024;
    if(argc > 1) {
        int mb = atoi(argv[1]);
        if(mb <= 0 || mb > 4095) {
            printf("dmatest: FAIL bad size '%s' (want 1..4095 MB)\n", argv[1]);
            return 1;
        }
        bytes = (uint64_t)mb * 1024 * 1024;
    }
    uint32_t psize = info.page_size;
    uint32_t size = (uint32_t)(bytes / psize) * psize;
    printf("dmatest: requesting %u bytes (%.2f GB) from dma block 0\n",
            size, size / (double)GB);

    printf("dmatest: sys_dma range [0x%llx, 0x%llx)\n",
            (unsigned long long)info.sys_dma.phy_base,
            (unsigned long long)(info.sys_dma.phy_base + info.sys_dma.size));
    if((uint64_t)size > info.sys_dma.size) {
        printf("dmatest: FAIL request larger than the sys_dma range\n");
        return 1;
    }

    ewokos_addr_t vaddr = dma_alloc(0, size);
    if(vaddr == 0) {
        printf("dmatest: FAIL dma_alloc(%u) returned 0\n", size);
        return 1;
    }
    ewokos_addr_t paddr = dma_phy_addr(0, vaddr);
    printf("dmatest: vaddr=0x%llx paddr=0x%llx\n",
            (unsigned long long)vaddr, (unsigned long long)paddr);
    if(paddr == 0 || (paddr & (psize - 1)) != 0) {
        printf("dmatest: FAIL bad physical base\n");
        return 1;
    }
    if(paddr < info.sys_dma.phy_base ||
            paddr + size > info.sys_dma.phy_base + info.sys_dma.size) {
        printf("dmatest: FAIL run outside the sys_dma range\n");
        return 1;
    }

    uint32_t* buf = (uint32_t*)(uintptr_t)vaddr;
    uint32_t words = size / 4;

    printf("dmatest: writing pattern over %u words...\n", words);
    for(uint32_t i = 0; i < words; i++) {
        buf[i] = pattern(i);
        if(i != 0 && (i & 0x3FFFFFF) == 0) /* every 256MB */
            printf("dmatest: write %u MB\n",
                    (uint32_t)((uint64_t)i * 4 / (1024 * 1024)));
    }

    printf("dmatest: verifying %u words...\n", words);
    for(uint32_t i = 0; i < words; i++) {
        uint32_t got = buf[i];
        if(got != pattern(i)) {
            printf("dmatest: FAIL at offset 0x%x: got 0x%08x want 0x%08x\n",
                    i * 4, got, pattern(i));
            return 1;
        }
        if(i != 0 && (i & 0x3FFFFFF) == 0)
            printf("dmatest: verify %u MB\n",
                    (uint32_t)((uint64_t)i * 4 / (1024 * 1024)));
    }

    dma_free(0, vaddr);
    printf("dmatest: freed, re-allocating 1MB to prove release...\n");
    ewokos_addr_t v2 = dma_alloc(0, 1024 * 1024);
    if(v2 == 0) {
        printf("dmatest: FAIL re-alloc after free returned 0\n");
        return 1;
    }
    uint32_t* small = (uint32_t*)(uintptr_t)v2;
    for(uint32_t i = 0; i < 1024 * 1024 / 4; i++)
        small[i] = pattern(i);
    for(uint32_t i = 0; i < 1024 * 1024 / 4; i++) {
        if(small[i] != pattern(i)) {
            printf("dmatest: FAIL re-alloc verify at 0x%x\n", i * 4);
            return 1;
        }
    }
    dma_free(0, v2);

    printf("dmatest: PASS (%u bytes alloc, placed, verified, freed)\n", size);
    return 0;
}