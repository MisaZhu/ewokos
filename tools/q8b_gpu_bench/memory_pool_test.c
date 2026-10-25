#include <assert.h>
#include <stdio.h>
#include "../../machines/q8b/kernel/bsp/memory_pool.h"

static const q8b_mem_range_t split[] = {
    {0xe0000000, 0xf0000000}, {0xc8600000, 0xd0000000},
    {0xd8000000, 0xe0000000}, {0xd0000000, 0xd8000000},
    {0xf1000000, 0xf8000000}
};
static int32_t read_range(uint32_t i, uint64_t* base, uint64_t* top) {
    *base = split[i].base; *top = split[i].top;
    return 1;
}

static uint64_t oracle(uint64_t base, uint64_t top, uint64_t size,
        const q8b_mem_range_t* ex, uint32_t n) {
    for (uint64_t p = (base + 15) & ~15ULL; p < top && size <= top - p; p += 16) {
        int free = 1;
        for (uint32_t i = 0; i < n; i++)
            if (ex[i].top > ex[i].base && ex[i].base < p + size && ex[i].top > p)
                free = 0;
        if (free) return p;
    }
    return 0;
}

int main(void) {
    /* A 512MB pool spans several UEFI allocation types. Merge unsorted
     * touching descriptors, but stop at the missing f0000000..f1000000. */
    assert(q8b_pool_extend(0xd0000000, 5, read_range) == 0xf0000000);
    /* Actual failing layout: SHM crosses firmware-owned RAM and the bank
     * hole. It must move to the usable second bank, not just past bb700000. */
    q8b_mem_range_t ex[] = {{0xaeb00000, 0xc0000000},
            {0xc6200000, 0xc8600000}, {0x987a8000, 0xa87a8000},
            {0xffec2000, 0xffed2000}};
    assert(!q8b_pool_candidate(0xa87a8000, 0xc0000000, 512ULL<<20, 16384, ex, 4));
    assert(q8b_pool_candidate(0xc0000000, 0xffec2000, 512ULL<<20, 16384, ex, 4)
            == 0xc8600000);
    assert(!q8b_pool_candidate(UINT64_MAX-7, UINT64_MAX, 16, 16, ex, 4));
    assert(!q8b_pool_candidate(16, 1024, 17, 16, ex, 4));
    assert(!q8b_pool_candidate(16, 1024, 16, 3, ex, 4));
    assert(q8b_pool_candidate(17, 64, 32, 16, ex, 0) == 32);
    assert(!q8b_pool_candidate(17, 63, 32, 16, ex, 0));
    uint32_t seed = 20261026;
    for (uint32_t t = 0; t < 20000; t++) {
        q8b_mem_range_t random[8];
        for (uint32_t i = 0; i < 8; i++) {
            seed = seed * 1664525u + 1013904223u;
            random[i].base = seed % 2048;
            seed = seed * 1664525u + 1013904223u;
            random[i].top = random[i].base + seed % 400;
        }
        uint64_t base = 1 + t % 127, size = (1 + t % 32) * 16;
        assert(q8b_pool_candidate(base, 2048, size, 16, random, 8)
                == oracle(base, 2048, size, random, 8));
    }
    puts("memory pool regression: physical hole, exclusions, alignment, overflow and 20000 oracle cases passed");
}
