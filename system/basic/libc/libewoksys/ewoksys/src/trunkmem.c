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

static mem_block_t* gen_block(char* p, uint32_t size) {
    uint32_t block_size = sizeof(mem_block_t);
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
static void try_break(malloc_t* m, mem_block_t* block, uint32_t size) {
    uint32_t block_size = sizeof(mem_block_t);
    //required more than half size of block. no break.
    if((block_size+size) > (uint32_t)(block->size/2)) 
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

    /* a new free block just appeared: keep the free_max invariant */
    if(newBlock->size > m->free_max)
        m->free_max = newBlock->size;
}

/* Recompute free_max by walking the whole chain. Only called on the rare
 * paths where the largest free block may have disappeared (tail shrink, or
 * a first-fit walk that came up empty against an overestimated free_max). */
static void trunk_rescan_free_max(malloc_t* m) {
    uint32_t max = 0;
    mem_block_t* block = m->head;
    while(block != NULL) {
        if(!block->used && block->size > max)
            max = block->size;
        block = block->next;
    }
    m->free_max = max;
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

char* trunk_malloc(malloc_t* m, uint32_t size) {
    mem_block_t* head;
    mem_block_t* prev;
    mem_block_t* block;
    ewokos_addr_t heap_end;
    if(m == NULL)
        return NULL;

    trunk_lock_heap(m);
    size = ALIGN_UP(size, 8);
    heap_end = trunk_heap_end(m);
    /* No free block can satisfy this request: skip the first-fit walk
     * entirely and expand. Without this short-circuit every allocation on a
     * nearly-full heap walked the whole chain (O(block_count) with a
     * per-block sanity check), which turned allocation-heavy workloads
     * quadratic -- e.g. xBrowser parsing a CSS-heavy page spent minutes
     * inside trunk_block_sane with hundreds of thousands of live blocks. */
    if(size > m->free_max && m->head != NULL) {
        /* free_max may only overestimate after consumes/shrinks are handled
         * below, so when it claims "too small" the walk is provably futile. */
    } else if(m->head != NULL) {
        head = m->head;
        prev = NULL;
        block = head;
        if(m->start != NULL) {
            /* validate the rotate hint in O(1) instead of scanning from head;
            on any inconsistency fall back to head (walk re-validates anyway) */
            mem_block_t* sprev = NULL;
            if(trunk_check_block(m, m->start, heap_end, &sprev) != NULL) {
                block = m->start;
                prev = sprev;
            }
        }
        while(block != NULL) {
            /* heap_end is hoisted out of the loop: get_mem_tail is an indirect
             * call into libgloss, so re-deriving it per block made the walk
             * far more expensive than the sanity check itself. */
            if(!trunk_block_sane(head, heap_end, prev, block)) {
                trunk_unlock_heap(m);
                return NULL;
            }
            if(block->used || block->size < size) {
                prev = block;
                block = block->next;
            }
            else {
                block->used = 1;
                if(block->size == m->free_max &&
                        (sizeof(mem_block_t)+size) > (uint32_t)(block->size/2)) {
                    /* The largest free block was consumed whole (no break
                     * will re-publish a remainder): rescan so free_max stays
                     * exact and the next futile walk is short-circuited. */
                    try_break(m, block, size);
                    trunk_rescan_free_max(m);
                } else {
                    try_break(m, block, size);
                }
                m->start = block->next;
                trunk_unlock_heap(m);
                return block->mem;
            }
        }
        /* Walk came up empty although free_max promised a fit: the invariant
         * overestimated (stale from merges/shrinks). Recompute it so the next
         * oversized request takes the expand fast path. */
        trunk_rescan_free_max(m);
        if(size > m->free_max) {
            /* fall through to expand */
        }
    }

    /*Can't find any available block, expand pages*/
    uint32_t block_size = sizeof(mem_block_t);
    uint32_t expand_size = size + block_size;

    uint32_t pages = expand_size / m->seg_size;	
    if((expand_size % m->seg_size) > 0)
        pages++;

    char* p = (char*)m->get_mem_tail(m->arg);
    if(m->expand(m->arg, pages) != 0) {
        trunk_unlock_heap(m);
        return NULL;
    }

    block = gen_block(p, pages*m->seg_size);
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

    uint32_t pages = (m->tail->size+block_size) / m->seg_size;
    /* The largest free block may be the one being returned to the kernel:
     * unlink it first, then recompute the invariant so free_max never
     * advertises a block that is already gone. */
    int rescan = (m->tail->size >= m->free_max);
    m->tail = m->tail->prev;
    if(m->tail != NULL)
        m->tail->next = NULL;
    else
        m->head = NULL;
    m->shrink(m->arg, pages);
    if(rescan)
        trunk_rescan_free_max(m);
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
    /* A (possibly merged) free block just grew: keep the free_max invariant. */
    if(block->size > m->free_max)
        m->free_max = block->size;
    if(m->start == 0 || m->start >= block)
        m->start = block->prev;
    if(m->shrink != NULL)
        try_shrink(m);
    trunk_unlock_heap(m);
}

uint32_t trunk_msize(malloc_t* m, char* p) {
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

    uint32_t size = block->size;
    trunk_unlock_heap(m);
    return size;
}

void trunk_stat(malloc_t* m, uint32_t* blocks, uint32_t* free_blocks,
        uint32_t* used_bytes, uint32_t* free_bytes) {
    uint32_t nblocks = 0;
    uint32_t nfree = 0;
    uint32_t used = 0;
    uint32_t freem = 0;

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
        trunk_unlock_heap(m);
    }

    if(blocks != NULL)
        *blocks = nblocks;
    if(free_blocks != NULL)
        *free_blocks = nfree;
    if(used_bytes != NULL)
        *used_bytes = used;
    if(free_bytes != NULL)
        *free_bytes = freem;
}

#ifdef __cplusplus
}
#endif
