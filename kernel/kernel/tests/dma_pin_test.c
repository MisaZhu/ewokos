/* Production allocator; fake process table, page-table revoke and kmalloc. */
#include <mm/dma.h>
#include <mm/kmalloc.h>
#include <kernel/hw_info.h>
#include <kernel/proc.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BASE 0x20000000ull
#define PAGES 8u
sys_info_t _sys_info;
static proc_t procs[64];
static proc_space_t peer_space;
static page_dir_entry_t peer_vm[PAGE_DIR_NUM];
static unsigned revokes, flushes;
static unsigned nodes;
void* kcalloc(uint32_t n, uint32_t size) {
    void *p = calloc(n, size); assert(p); nodes++; return p;
}
void kfree(void *p) { assert(p && nodes); nodes--; free(p); }
proc_t* proc_get(int32_t pid) {
    return pid > 0 && pid < 64 ? &procs[pid] : NULL;
}
void unmap_pages(page_dir_entry_t *vm, ewokos_addr_t va, uint32_t pages) {
    assert(vm == peer_vm && va == 0x60000000u && pages == 1);
    revokes++;
}
void flush_tlb(void) { flushes++; }

static void test_free_and_last_reference(void) {
    ewokos_addr_t a = dma_alloc(0, 11, PAGE_SIZE);
    ewokos_addr_t pinned = dma_alloc(0, 11, PAGE_SIZE);
    ewokos_addr_t c = dma_alloc(0, 11, PAGE_SIZE);
    assert(a == BASE && pinned == BASE + PAGE_SIZE && c == BASE + 2*PAGE_SIZE);
    assert(dma_pin(0, 11, pinned, PAGE_SIZE) == 0);
    assert(dma_pin(0, 11, pinned, 1) == 0);
    int32_t owner = -1;
    uint32_t pins = 0;
    assert(dma_pin_query(0, pinned, &owner, &pins) == 0 && owner == 11 && pins == 2);
    assert(dma_pin_query(0, pinned + 1, &owner, &pins) == -1);
    assert(dma_pin_query(-1, pinned, &owner, &pins) == -1);
    assert(dma_pin_query(0, pinned, NULL, &pins) == -1);
    dma_free(0, 11, pinned);
    assert(dma_pin_query(0, pinned, &owner, &pins) == 0 && owner == 0 && pins == 2);
    assert(dma_size(0, 11, pinned) == 0); /* no longer a live owner */
    assert(dma_pin(0, 11, pinned, 1) == -1);
    dma_free(0, 11, a); dma_free(0, 11, c);
    dma_release(11);
    ewokos_addr_t used[PAGES]; unsigned count = 0;
    for (;;) {
        ewokos_addr_t pa = dma_alloc(0, 22, PAGE_SIZE);
        if (!pa) break;
        assert(count < PAGES && pa != pinned);
        used[count++] = pa;
    }
    assert(count == PAGES - 1);
    assert(dma_unpin(0, pinned) == 0);
    assert(dma_alloc(0, 33, PAGE_SIZE) == 0); /* second device still owns it */
    dma_release(22);
    assert(dma_alloc(0, 33, PAGES * PAGE_SIZE) == 0);
    assert(dma_unpin(0, pinned) == 0);
    assert(dma_unpin(0, pinned) == -1);
    assert(dma_pin_query(0, pinned, &owner, &pins) == -1);
    assert(dma_alloc(0, 33, PAGES * PAGE_SIZE) == BASE);
    dma_release(33);
    (void)used;
    assert(nodes == 1);
    puts("PASS: free and coalescing exclude pinned storage; final device reference restores full pool");
}

static void test_exit_pid_reuse_and_peers(void) {
    ewokos_addr_t target = dma_alloc(0, 11, PAGE_SIZE);
    ewokos_addr_t rest = dma_alloc(0, 22, (PAGES-1)*PAGE_SIZE);
    assert(target == BASE && rest == BASE+PAGE_SIZE);
    assert(dma_pin(0, 11, target, PAGE_SIZE) == 0);
    assert(dma_pin(0, 11, target, PAGE_SIZE) == 0);
    procs[44].info.state = RUNNING;
    procs[44].space = &peer_space; peer_space.vm = peer_vm;
    assert(dma_peer_map_by_paddr(44, 0x60000000u, target, PAGE_SIZE) >= 0);
    dma_release(11);
    assert(revokes == 1 && flushes == 1);
    assert(dma_size(0, 11, target) == 0);
    assert(dma_alloc(0, 11, PAGE_SIZE) == 0); /* PID reuse is not ownership */
    dma_release(11);
    assert(revokes == 1 && flushes == 1);
    assert(dma_unpin(0, target) == 0);
    assert(dma_alloc(0, 11, PAGE_SIZE) == 0);
    assert(dma_unpin(0, target) == 0);
    assert(dma_alloc(0, 11, PAGE_SIZE) == target);
    assert(dma_size(0, 22, rest) == (PAGES-1)*PAGE_SIZE);
    dma_release(11); dma_release(22);
    assert(dma_alloc(0, 33, PAGES*PAGE_SIZE) == BASE);
    dma_free(0, 33, BASE);
    assert(nodes == 1);
    puts("PASS: owner exit revokes CPU peers but retains device storage across PID reuse and repeated release");
}

static void test_validation_and_live_owner(void) {
    ewokos_addr_t target = dma_alloc(0, 11, PAGE_SIZE);
    assert(dma_pin(-1,11,target,1) == -1);
    assert(dma_pin(16,11,target,1) == -1);
    assert(dma_pin(0,0,target,1) == -1);
    assert(dma_pin(0,22,target,1) == -1);
    assert(dma_pin(0,11,target,0) == -1);
    assert(dma_pin(0,11,target,PAGE_SIZE+1) == -1);
    assert(dma_pin(0,11,target+1,1) == -1);
    assert(dma_unpin(0,target) == -1);
    assert(dma_pin(0,11,target,PAGE_SIZE) == 0);
    assert(dma_unpin(-1,target) == -1 && dma_unpin(16,target) == -1);
    assert(dma_unpin(0,target+1) == -1);
    assert(dma_unpin(0,target) == 0);
    assert(dma_size(0,11,target) == PAGE_SIZE);
    assert(dma_alloc(0,22,(PAGES-1)*PAGE_SIZE) == target+PAGE_SIZE);
    assert(dma_alloc(0,33,PAGE_SIZE) == 0); /* final unpin must not free live owner */
    dma_free(0,11,target); dma_release(22);
    assert(dma_alloc(0,33,PAGES*PAGE_SIZE) == BASE);
    dma_release(33);
    puts("PASS: pin validation rejects foreign, freed, partial-base and oversized ranges; unpin preserves live owner");
}

static void test_registered_block_reclaim(void) {
    const ewokos_addr_t base = BASE + 16*PAGE_SIZE;
    int32_t block = dma_set(44,base,base,2*PAGE_SIZE,false);
    assert(block == 1);
    ewokos_addr_t target = dma_alloc(block,33,PAGE_SIZE);
    assert(target == base && dma_pin(block,33,target,PAGE_SIZE) == 0);
    dma_release(33); procs[44].info.state = UNUSED;
    assert(dma_set(55,base,base,2*PAGE_SIZE,false) == -1);
    assert(dma_alloc(block,55,2*PAGE_SIZE) == 0);
    assert(dma_unpin(block,target) == 0);
    assert(dma_set(55,base,base,2*PAGE_SIZE,false) == block);
    assert(dma_alloc(block,55,2*PAGE_SIZE) == base);
    dma_release(55);
    assert(nodes == 2);
    puts("PASS: dead block registration cannot discard device pins; reclaim works after final unpin");
}

static uint32_t rng_state = 0x51908280u;
static uint32_t random_word(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}
static void test_lifetime_sequences(void) {
    struct held { ewokos_addr_t pa; uint32_t size, pins; int32_t pid; int live; } held[PAGES] = {{0}};
    const int32_t owners[] = {11, 22, 33, -1};
    unsigned allocs = 0, pins = 0, frees = 0, exits = 0, unpins = 0;
    for (unsigned step = 0; step < 10000; step++) {
        unsigned index = random_word() % PAGES;
        struct held *h = &held[index];
        unsigned action = random_word() % 5;
        if (action == 0) {
            uint32_t size = (1 + random_word() % 3)*PAGE_SIZE;
            int32_t owner = owners[random_word() % 4];
            ewokos_addr_t pa = dma_alloc(0, owner, size);
            if (pa) {
                unsigned slot;
                assert(pa >= BASE && pa + size <= BASE + PAGES*PAGE_SIZE);
                for (slot = 0; slot < PAGES; slot++) {
                    const struct held *other = &held[slot];
                    if (!other->pa) continue;
                    assert(pa + size <= other->pa || other->pa + other->size <= pa);
                }
                for (slot = 0; slot < PAGES && held[slot].pa; slot++) {}
                assert(slot < PAGES);
                held[slot] = (struct held){pa,size,0,owner,1};
                allocs++;
            }
        } else if (action == 1 && h->pa && h->live) {
            assert(dma_pin(0,h->pid,h->pa,h->size) == 0);
            h->pins++; pins++;
        } else if (action == 2 && h->pa && h->live) {
            dma_free(0,h->pid,h->pa); h->live = 0; frees++;
        } else if (action == 3 && h->pa && h->pins) {
            assert(dma_unpin(0,h->pa) == 0); h->pins--; unpins++;
        } else if (action == 4) {
            int32_t owner = owners[random_word() % 4];
            dma_release(owner); exits++;
            for (unsigned j=0;j<PAGES;j++)
                if (held[j].pa && held[j].pid == owner) held[j].live = 0;
        }
        for (unsigned j=0;j<PAGES;j++) {
            struct held *item = &held[j];
            if (!item->pa) continue;
            if (!item->live && !item->pins) { memset(item,0,sizeof(*item)); continue; }
            assert(dma_size(0,item->pid,item->pa) == (item->live ? item->size : 0));
        }
    }
    for (unsigned j=0;j<PAGES;j++)
        while (held[j].pins) {
            assert(dma_unpin(0,held[j].pa) == 0); held[j].pins--;
        }
    for (unsigned j=0;j<4;j++) dma_release(owners[j]);
    assert(dma_alloc(0,11,PAGES*PAGE_SIZE) == BASE);
    dma_free(0,11,BASE);
    printf("PASS: 10000 lifetime operations; allocations=%u pins=%u frees=%u exits=%u unpins=%u; no live/pinned physical overlap\n",
        allocs,pins,frees,exits,unpins);
}

int main(void) {
    _sys_info.sys_dma.phy_base = BASE; _sys_info.sys_dma.v_base = BASE;
    _sys_info.sys_dma.size = PAGES*PAGE_SIZE;
    dma_init();
    test_free_and_last_reference();
    test_exit_pid_reuse_and_peers();
    test_validation_and_live_owner();
    test_lifetime_sequences();
    test_registered_block_reclaim();
    printf("PASS: production DMA allocator device references, page_size=%u; ASan/UBSan enabled\n", (unsigned)PAGE_SIZE);
    return 0;
}
