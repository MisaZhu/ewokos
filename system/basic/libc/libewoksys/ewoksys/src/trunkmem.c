#include <stddef.h>
#include <string.h>
#include <pthread.h>
#include <ewoksys/ewokdef.h>
#include <ewoksys/proc.h>
#include <ewoksys/trunkmem.h>

#ifdef __cplusplus
extern "C" {
#endif
/*
malloc for memory trunk management
*/

/*
 * Heap lock.
 *
 * malloc()/free() take this lock on every single call. It used to be a
 * pthread_mutex, which on EwokOS is backed by a kernel semaphore: each
 * lock/unlock pair cost two syscalls (SYS_SEMAPHORE_TRY_ENTER then
 * SYS_SEMAPHORE_QUIT). Allocation-heavy workloads therefore spent nearly
 * all their time trapping into the kernel -- e.g. litehtml building the
 * per-element CSS property std::map (a red-black tree that news/deletes one
 * node per property) pinned a core at 100% and froze xBrowser for tens of
 * seconds while loading a CSS-heavy page such as cleanpng.com.
 *
 * The guarded regions below are short and never block, so an uncontended
 * userspace atomic test-and-set is sufficient and costs no syscall.
 * Contention is rare (only the network worker allocates beside the main
 * render thread); when it does happen we spin briefly and then proc_yield(),
 * so a preempted holder can run instead of the waiter burning CPU.
 *
 * m->lock is a pthread_mutex_t, i.e. an int32_t; the heap struct is
 * zero-initialised (static + memset in compat_heap_init), so the spin flag
 * starts at 0 == unlocked and needs no lazy init.
 */
#define TRUNK_SPIN_YIELD_THRESHOLD 64

static inline void trunk_lock_heap(malloc_t* m) {
    volatile int32_t* lock;
    int spins = 0;

    if(m == NULL)
        return;

    lock = (volatile int32_t*)&m->lock;
    while(__sync_lock_test_and_set(lock, 1)) {
        if(++spins >= TRUNK_SPIN_YIELD_THRESHOLD) {
            spins = 0;
            proc_yield();
        }
    }
}

static inline void trunk_unlock_heap(malloc_t* m) {
    if(m == NULL)
        return;

    __sync_lock_release((volatile int32_t*)&m->lock);
}

static inline ewokos_addr_t trunk_heap_end(malloc_t* m) {
    if(m == NULL || m->get_mem_tail == NULL)
        return 0;
    return (ewokos_addr_t)m->get_mem_tail(m->arg);
}

static inline int trunk_ptr_aligned(const void* p) {
    return ((((ewokos_addr_t)p) & (sizeof(void*) - 1)) == 0);
}

static inline int trunk_ptr_in_heap(mem_block_t* head, ewokos_addr_t heap_end, const void* p) {
    ewokos_addr_t addr;
    if(head == NULL || p == NULL || heap_end == 0)
        return 0;
    addr = (ewokos_addr_t)p;
    return addr >= (ewokos_addr_t)head && addr < heap_end;
}

static int trunk_block_sane(mem_block_t* head, ewokos_addr_t heap_end,
        mem_block_t* prev, mem_block_t* block) {
    ewokos_addr_t block_addr;
    ewokos_addr_t mem_addr;
    ewokos_addr_t block_end;

    if(block == NULL)
        return 0;
    if(!trunk_ptr_in_heap(head, heap_end, block) || !trunk_ptr_aligned(block))
        return 0;

    block_addr = (ewokos_addr_t)block;
    mem_addr = (ewokos_addr_t)block->mem;
    if(mem_addr != (block_addr + sizeof(mem_block_t)))
        return 0;
    if(mem_addr > heap_end)
        return 0;

    block_end = mem_addr + block->size;
    if(block_end < mem_addr || block_end > heap_end)
        return 0;

    if(block->prev != prev)
        return 0;
    if(prev != NULL && prev->next != block)
        return 0;

    if(block->next != NULL) {
        ewokos_addr_t next_addr = (ewokos_addr_t)block->next;
        if(!trunk_ptr_in_heap(head, heap_end, block->next) || !trunk_ptr_aligned(block->next))
            return 0;
        if(next_addr <= block_addr || next_addr < block_end)
            return 0;
        if(block->next->prev != block)
            return 0;
    }

    return 1;
}

/* Free blocks chain through two pointers stored at the start of their (dead)
 * payload, so every allocation must be large enough to hold them. */
#define TRUNK_FREE_LINK_BYTES (2 * (uint32_t)sizeof(mem_block_t*))

static inline mem_block_t** fl_next_slot(mem_block_t* b) {
    return (mem_block_t**)(void*)b->mem;
}

static inline mem_block_t** fl_prev_slot(mem_block_t* b) {
    return (mem_block_t**)(void*)(b->mem + sizeof(mem_block_t*));
}

/* Class of a block, from its size: floor(log2(size)) - TRUNK_FREE_MIN_BITS,
 * clamped to the valid range. Class 0 also absorbs anything smaller than the
 * minimum allocation, and the top class is open-ended. Sizes are address-width
 * (63-bit payloads exist on aarch64), hence clzll. */
static inline int trunk_block_class(ewokos_addr_t size) {
    int c;
    if(size < (1u << TRUNK_FREE_MIN_BITS))
        return 0;
    c = (int)(63 - __builtin_clzll(size)) - TRUNK_FREE_MIN_BITS;
    if(c >= TRUNK_FREE_CLASSES)
        c = TRUNK_FREE_CLASSES - 1;
    return c;
}

/* Lowest class in which EVERY block is >= size, so its head can be taken with
 * no size check at all. That is what makes trunk_malloc O(1): it never has to
 * search a class for a block that fits. */
static inline int trunk_request_class(ewokos_addr_t size) {
    int c;
    if(size <= (1u << TRUNK_FREE_MIN_BITS))
        return 0;
    c = (int)(64 - __builtin_clzll(size - 1)) - TRUNK_FREE_MIN_BITS; /* ceil(log2) */
    if(c >= TRUNK_FREE_CLASSES)
        c = TRUNK_FREE_CLASSES - 1;
    return c;
}

/* Push a free block onto its size class (LIFO). LIFO matters: the remainder
 * carved off by the previous allocation lands at the head of its class, so the
 * next allocation of a similar size reuses it immediately. */
static void fl_push(malloc_t* m, mem_block_t* b) {
    mem_block_t** head = &m->free_class[trunk_block_class(b->size)];
    *fl_next_slot(b) = *head;
    *fl_prev_slot(b) = NULL;
    if(*head != NULL)
        *fl_prev_slot(*head) = b;
    *head = b;
}

/* Drop a block from its size class. Must be called while the block's payload
 * still holds the links and its size is still the one it was pushed with --
 * i.e. before it is merged into a neighbour, shrunk away, split, or handed out
 * to a caller. */
static void fl_unlink(malloc_t* m, mem_block_t* b) {
    mem_block_t** head = &m->free_class[trunk_block_class(b->size)];
    mem_block_t* nx = *fl_next_slot(b);
    mem_block_t* pv = *fl_prev_slot(b);
    if(pv != NULL)
        *fl_next_slot(pv) = nx;
    else
        *head = nx;
    if(nx != NULL)
        *fl_prev_slot(nx) = pv;
    *fl_next_slot(b) = NULL;
    *fl_prev_slot(b) = NULL;
}

static mem_block_t* gen_block(char* p, ewokos_addr_t size) {
    ewokos_addr_t block_size = sizeof(mem_block_t);
    mem_block_t* block = (mem_block_t*)p;
    block->next = block->prev = NULL;
    block->mem = p + block_size;
    block->size = size - block_size;
    return block;
}

mem_block_t* get_block(char* p) {
    if(p == NULL)
        return NULL;

    uint32_t block_size = sizeof(mem_block_t);
    if(((ewokos_addr_t)p) < (ewokos_addr_t)block_size)
        return NULL;

    mem_block_t* block = (mem_block_t*)(p - block_size);
    return block;
}

/*if block size much bigger than the size required, break to two blocks*/
static void try_break(malloc_t* m, mem_block_t* block, ewokos_addr_t size) {
    ewokos_addr_t block_size = sizeof(mem_block_t);
    //required more than half size of block. no break.
    if((block_size+size) > (ewokos_addr_t)(block->size/2))
        return;

    //do break;
    char* p = block->mem + size;
    mem_block_t* newBlock = gen_block(p, block->size - size);
    newBlock->used = 0; //break a new free block.

    block->size = size;
    newBlock->next = block->next;
    if(newBlock->next != NULL)
        newBlock->next->prev = newBlock;

    newBlock->prev = block;
    block->next = newBlock;

    if(m->tail == block)
        m->tail = newBlock;

    /* a new free block just appeared: publish it on its size class */
    fl_push(m, newBlock);
}

/* O(1) validation of a block candidate without scanning from head.
Returns the block's prev via prev_out when the block (and its backward
link) look sane, so callers can resume a walk from it directly. */
static mem_block_t* trunk_check_block(malloc_t* m, mem_block_t* b,
        ewokos_addr_t heap_end, mem_block_t** prev_out) {
    if(b == NULL || m->head == NULL)
        return NULL;
    if(!trunk_ptr_in_heap(m->head, heap_end, b) || !trunk_ptr_aligned(b))
        return NULL;

    mem_block_t* prev = b->prev;
    if(prev != NULL &&
            (!trunk_ptr_in_heap(m->head, heap_end, prev) ||
             !trunk_ptr_aligned(prev) || prev->next != b))
        return NULL; /* corrupt backward link */
    if(!trunk_block_sane(m->head, heap_end, prev, b))
        return NULL;
    if(prev_out != NULL)
        *prev_out = prev;
    return b;
}

char* trunk_malloc(malloc_t* m, ewokos_addr_t size) {
    mem_block_t* block;
    ewokos_addr_t heap_end;
    if(m == NULL)
        return NULL;

    trunk_lock_heap(m);
    /* 16-byte payload alignment (NEON/C++ minimums on this tree); every
       block is also large enough for the intrusive free-list links */
    size = ALIGN_UP(size, 16);
    if(size < TRUNK_FREE_LINK_BYTES)
        size = TRUNK_FREE_LINK_BYTES;
    heap_end = trunk_heap_end(m);

    /* Take a block from the size-class free lists -- never walk the physical
     * chain, which carries one node per live allocation and is overwhelmingly
     * used blocks (xBrowser on a CSS-heavy page: ~1.3M blocks). Searching that
     * chain cost milliseconds per allocation, so every tick overran its budget
     * and the xwin event loop starved.
     *
     * From trunk_request_class(size) upwards every block in a class is by
     * construction >= size, so taking a class head needs neither a size check
     * nor a search: allocation stays O(1) however fragmented the heap gets.
     * The single class below that may still hold a block big enough, so its
     * head gets one opportunistic check to keep internal fragmentation down. */
    {
        int c = trunk_block_class(size);
        int c_fit = trunk_request_class(size);
        if(c < c_fit) {
            mem_block_t* cur = m->free_class[c];
            if(cur != NULL && !cur->used && cur->size >= size &&
                    trunk_block_sane(m->head, heap_end, cur->prev, cur)) {
                fl_unlink(m, cur);
                cur->used = 1;
                try_break(m, cur, size);
                m->start = cur->next;
                trunk_unlock_heap(m);
                return cur->mem;
            }
        }
        for(c = c_fit; c < TRUNK_FREE_CLASSES; c++) {
            mem_block_t* cur = m->free_class[c];
            while(cur != NULL) {
                mem_block_t* nx = *fl_next_slot(cur);
                if(!cur->used && cur->size >= size &&
                        trunk_block_sane(m->head, heap_end, cur->prev, cur)) {
                    fl_unlink(m, cur);
                    cur->used = 1;
                    /* any remainder is republished on its own size class */
                    try_break(m, cur, size);
                    m->start = cur->next;
                    trunk_unlock_heap(m);
                    return cur->mem;
                }
                /* Only the open-ended top class can hold blocks that are too
                 * small for the request, so this inner loop runs at most once
                 * for every other class. */
                if(c != TRUNK_FREE_CLASSES - 1)
                    break;
                cur = nx;
            }
        }
        /* no class holds a block for this size: fall through and expand */
    }

    /*Can't find any available block, expand pages*/
    ewokos_addr_t block_size = sizeof(mem_block_t);
    ewokos_addr_t expand_size = size + block_size;

    ewokos_addr_t npages = expand_size / m->seg_size;
    if((expand_size % m->seg_size) > 0)
        npages++;
    /* the expand callback takes a int32_t page count */
    if(npages > 0x7fffffffu) {
        trunk_unlock_heap(m);
        return NULL;
    }
    uint32_t pages = (uint32_t)npages;

    char* p = (char*)m->get_mem_tail(m->arg);
    if(m->expand(m->arg, pages) != 0) {
        trunk_unlock_heap(m);
        return NULL;
    }

    block = gen_block(p, (ewokos_addr_t)pages*m->seg_size);
    block->used = 1;

    if(m->head == NULL) {
        m->head = block;
    }

    if(m->tail == NULL) {
        m->tail = block;
    }
    else {
        m->tail->next = block;
        block->prev = m->tail;
        m->tail = block;
    }

    try_break(m, block, size);
    m->start = block;
    trunk_unlock_heap(m);
    return block->mem;
}

/*
try to merge around free blocks.
*/
static mem_block_t* try_merge(malloc_t* m, mem_block_t* block) {
    mem_block_t* b;
    mem_block_t* ret = block;
    uint32_t block_size = sizeof(mem_block_t);
    ewokos_addr_t heap_end = trunk_heap_end(m);
    //try next block
    b = block->next;
    if(b != NULL && b->used == 0) {
        mem_block_t* bn = b->next;
        /* bn is one hop past the validated range; check it before linking
        (so a corrupt forward link can no longer be dereferenced) */
        if(bn != NULL &&
                (m->head == NULL ||
                 !trunk_ptr_in_heap(m->head, heap_end, bn) ||
                 !trunk_ptr_aligned(bn)))
            return ret; /* leave list untouched */
        /* b is about to disappear into block: drop it from the free list while
         * its payload still holds the links (afterwards those bytes belong to
         * the merged block's payload and a stale entry would be fatal). */
        fl_unlink(m, b);
        block->size += (b->size + block_size);
        block->next = bn;
        if(bn != NULL)
            bn->prev = block;
        else
            m->tail = block;
    }

    //try left block
    b = block->prev;
    if(b != NULL && b->used == 0) {
        fl_unlink(m, b); /* see above: b absorbs block, so b's links must go */
        b->size += (block->size + block_size);
        b->next = block->next;
        if(b->next != NULL)
            b->next->prev = b;
        else
            m->tail = b;
        ret = b;
    }

    return ret;
}

/*
try to shrink the pages.
*/
static void try_shrink(malloc_t* m) {
    uint32_t block_size = sizeof(mem_block_t);
    ewokos_addr_t addr = (ewokos_addr_t)m->tail;
    //check if page aligned.
    if(m->tail == NULL ||
            m->tail->used == 1 ||
            (addr % (ewokos_addr_t)m->seg_size) != 0)
        return;

    uint32_t pages = (uint32_t)((m->tail->size+block_size) / m->seg_size);
    /* This block is about to be returned to the kernel: drop it from its size
     * class first, while its payload still holds the links and its size is
     * still the one it was pushed with. Leaving it on the list would hand out
     * unmapped memory on the next allocation. */
    fl_unlink(m, m->tail);
    m->tail = m->tail->prev;
    if(m->tail != NULL)
        m->tail->next = NULL;
    else
        m->head = NULL;
    m->shrink(m->arg, pages);
}

void trunk_free(malloc_t* m, char* p) {
    if(m == NULL)
        return;

    trunk_lock_heap(m);
    /* locate and validate the block in O(1) (no scan from head): with many
    small heap blocks the old from-head validation scan made every free
    O(block_count), turning alloc-heavy workloads quadratic. */
    mem_block_t* block = trunk_check_block(m, get_block(p), trunk_heap_end(m), NULL);
    if(block == NULL) {
        trunk_unlock_heap(m);
        return;
    }
    if(block->used == 0) {
        trunk_unlock_heap(m);
        return;
    }

    block->used = 0; //mark as free.
    block = try_merge(m, block);
    if(block == NULL) {
        trunk_unlock_heap(m);
        return;
    }
    /* Publish the (possibly merged) free block on the size class matching its
     * new, post-merge size. */
    fl_push(m, block);
    if(m->start == 0 || m->start >= block)
        m->start = block->prev;
    if(m->shrink != NULL)
        try_shrink(m);
    trunk_unlock_heap(m);
}

ewokos_addr_t trunk_msize(malloc_t* m, char* p) {
    if(m == NULL)
        return 0;

    trunk_lock_heap(m);
    /* O(1) like trunk_free(): the block header sits right in front of p, so
     * validating it locally is enough. This used to walk the whole chain from
     * the head, making realloc() O(block_count) -- and since std::string and
     * std::vector grow through realloc, an allocation-heavy workload such as
     * litehtml parsing CSS became quadratic (hundreds of thousands of live
     * blocks x a per-block sanity check on every growth step). */
    mem_block_t* block = trunk_check_block(m, get_block(p), trunk_heap_end(m), NULL);
    if(block == NULL) {
        trunk_unlock_heap(m);
        return 0;
    }

    ewokos_addr_t size = block->size;
    trunk_unlock_heap(m);
    return size;
}

void trunk_stat(malloc_t* m, uint32_t* blocks, uint32_t* free_blocks,
        uint32_t* used_bytes, uint32_t* free_bytes,
        uint32_t* free_list_len, uint32_t* free_list_max) {
    uint32_t nblocks = 0;
    uint32_t nfree = 0;
    ewokos_addr_t used = 0;
    ewokos_addr_t freem = 0;
    uint32_t nlist = 0;
    ewokos_addr_t listmax = 0;

    if(m != NULL) {
        mem_block_t* block;
        trunk_lock_heap(m);
        block = m->head;
        while(block != NULL) {
            nblocks++;
            if(block->used)
                used += block->size;
            else {
                nfree++;
                freem += block->size;
            }
            block = block->next;
        }
        /* Walk every size class: the total must match the free count found on
         * the physical chain above, otherwise a class list drifted out of sync
         * (a block changed size or disappeared while still linked). */
        for(int c = 0; c < TRUNK_FREE_CLASSES && nlist <= nblocks + 1; c++) {
            block = m->free_class[c];
            while(block != NULL && nlist <= nblocks + 1) {
                nlist++;
                if(block->size > listmax)
                    listmax = block->size;
                block = *fl_next_slot(block);
            }
        }
        trunk_unlock_heap(m);
    }

    if(blocks != NULL)
        *blocks = nblocks;
    if(free_blocks != NULL)
        *free_blocks = nfree;
    if(used_bytes != NULL)
        *used_bytes = (uint32_t)used;
    if(free_bytes != NULL)
        *free_bytes = (uint32_t)freem;
    if(free_list_len != NULL)
        *free_list_len = nlist;
    if(free_list_max != NULL)
        *free_list_max = (uint32_t)listmax;
}

#ifdef __cplusplus
}
#endif
