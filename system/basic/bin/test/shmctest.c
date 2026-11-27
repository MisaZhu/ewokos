#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <ewoksys/shm.h>
#include <ewoksys/dma.h>
#include <ewoksys/sys.h>

/*
 * Shm test for the unified architecture: EVERY segment draws a
 * physically-contiguous run from general RAM inside the DMA-reachable
 * range (mm_alloc_contig, the same mechanism dma_alloc uses over
 * sys_dma), not from a fixed carve-out. Verify the run is contiguous,
 * placed inside allocable RAM and zeroed; write/read a position-dependent
 * pattern; free the segment and prove the pages went back to the allocator
 * by handing them to dma_alloc; then re-create a segment and prove it comes
 * back zeroed (no stale alias of the previous owner's data).
 *
 * Usage: shmctest [MB]  - request size in MB (1..512); default 32MB.
 */

static uint32_t pattern(uint32_t i) {
    return i * 2654435761u + 0x5A5A5A5Au;
}

static int fill_verify(uint32_t* buf, uint32_t words, const char* tag) {
    for(uint32_t i = 0; i < words; i++)
        buf[i] = pattern(i);
    for(uint32_t i = 0; i < words; i++) {
        if(buf[i] != pattern(i)) {
            printf("shmctest: FAIL %s verify at 0x%x: got 0x%08x want 0x%08x\n",
                    tag, i * 4, buf[i], pattern(i));
            return -1;
        }
    }
    return 0;
}

static int expect_zero(uint32_t* buf, uint32_t words, const char* tag) {
    for(uint32_t i = 0; i < words; i++) {
        if(buf[i] != 0) {
            printf("shmctest: FAIL %s not zeroed at 0x%x: 0x%08x\n",
                    tag, i * 4, buf[i]);
            return -1;
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {
    setbuf(stdout, NULL);

    sys_info_t info;
    if(sys_get_sys_info(&info) != 0) {
        printf("shmctest: FAIL sys_get_sys_info\n");
        return 1;
    }
    printf("shmctest: allocable RAM [0x%llx, 0x%llx), sys_dma range %llu KB\n",
            (unsigned long long)info.allocable_phy_mem_base,
            (unsigned long long)info.allocable_phy_mem_top,
            (unsigned long long)(info.sys_dma.size / 1024));

    uint64_t bytes = 32ull * 1024 * 1024;
    if(argc > 1) {
        int mb = atoi(argv[1]);
        if(mb <= 0 || mb > 512) {
            printf("shmctest: FAIL bad size '%s' (want 1..512 MB)\n", argv[1]);
            return 1;
        }
        bytes = (uint64_t)mb * 1024 * 1024;
    }
    uint32_t psize = info.page_size;
    uint32_t size = (uint32_t)(bytes / psize) * psize;
    uint32_t words = size / 4;
    if((uint64_t)size > info.allocable_phy_mem_top - info.allocable_phy_mem_base) {
        printf("shmctest: FAIL request larger than allocable RAM\n");
        return 1;
    }

    /* 1. create + attach a segment (every shm run is contiguous) */
    int id = shmget(IPC_PRIVATE, (int)size, 0666 | IPC_CREAT);
    if(id <= 0) {
        printf("shmctest: FAIL shmget(%u) returned %d\n", size, id);
        return 1;
    }
    uint32_t* buf = (uint32_t*)shmat(id, NULL, 0);
    if(buf == NULL || buf == (void*)-1) {
        printf("shmctest: FAIL shmat\n");
        return 1;
    }
    if(expect_zero(buf, words, "fresh segment") != 0)
        return 1;

    /* 2. physical placement: inside allocable RAM, page-aligned, contiguous */
    ewokos_addr_t paddr = shm_phy_addr(id, (ewokos_addr_t)(uintptr_t)buf);
    ewokos_addr_t plast = shm_phy_addr(id,
            (ewokos_addr_t)(uintptr_t)buf + size - psize);
    printf("shmctest: vaddr=0x%llx paddr=0x%llx last-page paddr=0x%llx\n",
            (unsigned long long)(uintptr_t)buf,
            (unsigned long long)paddr, (unsigned long long)plast);
    if(paddr == 0 || (paddr & (psize - 1)) != 0 ||
            paddr < info.allocable_phy_mem_base ||
            paddr + size > info.allocable_phy_mem_top) {
        printf("shmctest: FAIL run outside allocable RAM\n");
        return 1;
    }
    if(plast != paddr + size - psize) {
        printf("shmctest: FAIL run not physically contiguous\n");
        return 1;
    }

    /* 3. data integrity through the NOCACHE window */
    if(fill_verify(buf, words, "shm") != 0)
        return 1;
    printf("shmctest: %u bytes written/verified through the shm window\n", size);

    /* 4. free the segment: the run must return to general RAM.
       EwokOS destroys a segment on its LAST detach (shm_proc_unmap_it frees
       when refs drops to 0), so after shmdt the id must already be gone -
       a follow-up IPC_RMID would fail with "no such segment". */
    if(shmdt(buf) != 0) {
        printf("shmctest: FAIL shmdt\n");
        return 1;
    }
    if(shmctl(id, IPC_RMID, NULL) != -1) {
        printf("shmctest: FAIL segment still alive after last detach\n");
        return 1;
    }
    printf("shmctest: segment freed, handing the pages to dma_alloc...\n");

    /* 5. dma_alloc from the same general RAM: succeeds only if the freed
       pages are back in the allocator (a reserved slab could never serve
       DMA). Clamped to the sys_dma window, which bounds dma_alloc runs. */
    uint32_t dsize = size;
    if((uint64_t)dsize > info.sys_dma.size)
        dsize = (uint32_t)(info.sys_dma.size / psize) * psize;
    if(dsize > 0) {
        ewokos_addr_t dv = dma_alloc(dsize);
        if(dv == 0) {
            printf("shmctest: FAIL dma_alloc(%u) after shm free returned 0\n", dsize);
            return 1;
        }
        ewokos_addr_t dp = dma_phy_addr(dv);
        printf("shmctest: dma paddr=0x%llx (freed shm run was 0x%llx)%s\n",
                (unsigned long long)dp, (unsigned long long)paddr,
                (dp >= paddr && dp < paddr + size) ? " - REUSED the freed run" : "");
        if(fill_verify((uint32_t*)(uintptr_t)dv, dsize / 4, "dma reuse") != 0)
            return 1;
        dma_free(dv);
    }

    /* 6. re-create a segment: must come back zeroed (the previous
       owner's pattern must not survive through a stale alias) */
    int id2 = shmget(IPC_PRIVATE, (int)size, 0666 | IPC_CREAT);
    if(id2 <= 0) {
        printf("shmctest: FAIL re-create shmget returned %d\n", id2);
        return 1;
    }
    uint32_t* buf2 = (uint32_t*)shmat(id2, NULL, 0);
    if(buf2 == NULL || buf2 == (void*)-1) {
        printf("shmctest: FAIL re-create shmat\n");
        return 1;
    }
    if(expect_zero(buf2, words, "re-created segment") != 0)
        return 1;
    if(fill_verify(buf2, words, "re-created shm") != 0)
        return 1;
    shmdt(buf2); /* last detach frees the segment */

    printf("shmctest: PASS (%u bytes contig alloc, placed, verified, freed, reused)\n",
            size);
    return 0;
}