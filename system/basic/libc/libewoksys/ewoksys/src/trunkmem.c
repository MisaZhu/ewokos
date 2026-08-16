#include <stddef.h>
#include <string.h>
#include <pthread.h>
#include <ewoksys/ewokdef.h>
#include <ewoksys/trunkmem.h>

#ifdef __cplusplus
extern "C" {
#endif
/*
malloc for memory trunk management
*/

static pthread_mutex_t trunkmem_init_lock = 0;

static inline void trunk_ensure_lock(malloc_t* m) {
    if(m == NULL || m->lock_inited != 0)
        return;

    pthread_mutex_lock(&trunkmem_init_lock);
    if(m->lock_inited == 0) {
        pthread_mutex_init(&m->lock, NULL);
        m->lock_inited = 1;
    }
    pthread_mutex_unlock(&trunkmem_init_lock);
}

static inline void trunk_lock_heap(malloc_t* m) {
    if(m == NULL)
        return;

    trunk_ensure_lock(m);
    pthread_mutex_lock(&m->lock);
}

static inline void trunk_unlock_heap(malloc_t* m) {
    if(m == NULL || m->lock_inited == 0)
        return;

    pthread_mutex_unlock(&m->lock);
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

static mem_block_t* trunk_find_block_locked(malloc_t* m, mem_block_t* target) {
    mem_block_t* head;
    mem_block_t* prev;
    mem_block_t* block;
    ewokos_addr_t heap_end;

    if(m == NULL || target == NULL)
        return NULL;

    head = m->head;
    heap_end = trunk_heap_end(m);
    if(head == NULL || heap_end == 0)
        return NULL;

    prev = NULL;
    block = head;
    while(block != NULL) {
        if(!trunk_block_sane(head, heap_end, prev, block))
            return NULL;
        if(block == target)
            return block;
        prev = block;
        block = block->next;
    }

    return NULL;
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
    if(m == NULL)
        return NULL;

    trunk_lock_heap(m);
    size = ALIGN_UP(size, 8);
    head = m->head;
    prev = NULL;
    mem_block_t* block = head;
    if(m->start != NULL) {
        /* validate the rotate hint in O(1) instead of scanning from head;
        on any inconsistency fall back to head (walk re-validates anyway) */
        mem_block_t* sprev = NULL;
        if(trunk_check_block(m, m->start, trunk_heap_end(m), &sprev) != NULL) {
            block = m->start;
            prev = sprev;
        }
    }
    while(block != NULL) {
        if(!trunk_block_sane(head, trunk_heap_end(m), prev, block)) {
            trunk_unlock_heap(m);
            return NULL;
        }
        if(block->used || block->size < size) {
            prev = block;
            block = block->next;
        }
        else {
            block->used = 1;
            try_break(m, block, size);
            break;
        }
    }
    if(block != NULL) {
        m->start = block->next;
        trunk_unlock_heap(m);
        return block->mem;
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
    mem_block_t* block = trunk_find_block_locked(m, get_block(p));
    if(block == NULL) {
        trunk_unlock_heap(m);
        return 0;
    }

    uint32_t size = block->size;
    trunk_unlock_heap(m);
    return size;
}


#ifdef __cplusplus
}
#endif
