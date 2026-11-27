#include <mm/kalloc.h>
#include <mm/dma.h>
#include <mm/kmalloc.h>
#include <kernel/kernel.h>
#include <mm/mmu.h>
#include <kernel/system.h>
#include <kernel/hw_info.h>
#include <kstring.h>
#include <stddef.h>

pages_ref_t _pages_ref;

inline uint32_t page_ref_index(ewokos_addr_t paddr) {
    return ((paddr) - _pages_ref.phy_base) / PAGE_SIZE;
}

/*physical memory split to pages for paging mmu, managed by kalloc/kfree, phymem page state must be occupied or free*/

/*
 * _list of pages that are free to be allocated by kalloc. Each of the
 * list nodes are stored in the beginning of the actual page, because
 * the page is free and we can use it for our purposes.
 */
static __attribute__((__aligned__(PAGE_DIR_SIZE))) page_list_t *_free_list_page = 0;
static __attribute__((__aligned__(1024))) page_list_t *_free_list1k = 0;
static ewokos_addr_t _free_mem_size_page = 0;

/*
 * Physical-page occupancy bitmap mirroring the _free_list_page free list,
 * indexed exactly like _pages_ref (page_ref_index over the allocable span).
 * bit == 1 means the page is handed out (or split for 1k chunks), or was
 * never added to the free list at all (reserved slabs / firmware holes);
 * bit == 0 means it sits in the free list. The free list stays the source of
 * truth for kalloc_page()/kfree_page(); the bitmap only exists so contiguous-
 * run scans (kalloc_contig) and compaction can reason about physical
 * adjacency, which the unordered LIFO free list cannot express. Pages
 * outside the allocable span (kernel page-table pool, kmalloc) are not
 * tracked.
 */
static uint8_t* _page_busy = 0;
static uint32_t _page_busy_max = 0;

void kalloc_busy_init(uint32_t max_pages) {
    _page_busy_max = max_pages;
    _page_busy = (uint8_t*)kmalloc((max_pages + 7) / 8);
    /* Start ALL-BUSY: only pages actually appended to the free list
     * (kalloc_append/kfree_page clear their bit) may be claimed by contiguous
     * scans. Reserved slabs and firmware holes inside the span are never
     * appended, so they must stay invisible to kalloc_contig - a zero-init
     * bitmap would advertise them as free runs and the unlink pass would
     * splice non-member pages, corrupting the free list. */
    if(_page_busy != NULL)
        memset(_page_busy, 0xff, (max_pages + 7) / 8);
}

static inline uint32_t busy_index(void* page) {
    return page_ref_index(V2P((ewokos_addr_t)page));
}

static inline int busy_test(uint32_t i) {
    return (_page_busy[i >> 3] >> (i & 7)) & 1;
}

static inline void busy_set(uint32_t i) {
    _page_busy[i >> 3] |= (uint8_t)(1u << (i & 7));
}

static inline void busy_clear(uint32_t i) {
    _page_busy[i >> 3] &= (uint8_t)~(1u << (i & 7));
}

static inline void busy_mark(void* page, int occupied) {
    if(_page_busy == NULL)
        return;
    uint32_t i = busy_index(page);
    if(i >= _page_busy_max)
        return;
    if(occupied)
        busy_set(i);
    else
        busy_clear(i);
}

static page_list_t *page_list_prepend(page_list_t *page_list, char *page_address);

#define KALLOC_1K_CHUNK_SIZE KB
#define KALLOC_1K_CHUNKS_PER_PAGE (PAGE_SIZE / KALLOC_1K_CHUNK_SIZE)
#define KALLOC_1K_META_BUCKETS 64

typedef struct SplitPageMeta {
    struct SplitPageMeta *next;
    ewokos_addr_t page_base;
    uint32_t free_mask;
    uint32_t free_count;
} split_page_meta_t;

static split_page_meta_t *_split_page_meta_buckets[KALLOC_1K_META_BUCKETS];
static split_page_meta_t *_split_page_meta_free = 0;

static uint32_t split_page_meta_hash(ewokos_addr_t page_base) {
    return (uint32_t)((page_base / PAGE_SIZE) & (KALLOC_1K_META_BUCKETS - 1));
}

static split_page_meta_t *split_page_meta_find(ewokos_addr_t page_base) {
    uint32_t bucket = split_page_meta_hash(page_base);
    split_page_meta_t *meta = _split_page_meta_buckets[bucket];

    while (meta != NULL) {
        if (meta->page_base == page_base)
            return meta;
        meta = meta->next;
    }
    return NULL;
}

static int split_page_meta_grow(void) {
    char *page = kalloc_page();
    uint32_t i;
    uint32_t num;

    if (page == NULL)
        return -1;

    num = PAGE_SIZE / sizeof(split_page_meta_t);
    for (i = 0; i < num; i++) {
        split_page_meta_t *meta =
            (split_page_meta_t *)(page + i * sizeof(split_page_meta_t));
        meta->next = _split_page_meta_free;
        _split_page_meta_free = meta;
    }
    return 0;
}

static split_page_meta_t *split_page_meta_alloc(ewokos_addr_t page_base) {
    split_page_meta_t *meta;
    uint32_t bucket;

    if (_split_page_meta_free == NULL && split_page_meta_grow() != 0)
        return NULL;

    meta = _split_page_meta_free;
    _split_page_meta_free = meta->next;

    meta->page_base = page_base;
    meta->free_mask = 0;
    meta->free_count = 0;

    bucket = split_page_meta_hash(page_base);
    meta->next = _split_page_meta_buckets[bucket];
    _split_page_meta_buckets[bucket] = meta;
    return meta;
}

static void split_page_meta_free(split_page_meta_t *meta) {
    uint32_t bucket = split_page_meta_hash(meta->page_base);
    split_page_meta_t **current = &_split_page_meta_buckets[bucket];

    while (*current != NULL) {
        if (*current == meta) {
            *current = meta->next;
            meta->next = _split_page_meta_free;
            _split_page_meta_free = meta;
            return;
        }
        current = &(*current)->next;
    }
}

static inline ewokos_addr_t kalloc1k_page_base(void *mem) {
    return ALIGN_DOWN((ewokos_addr_t)mem, PAGE_SIZE);
}

static inline uint32_t kalloc1k_chunk_index(void *mem) {
    return (uint32_t)(((ewokos_addr_t)mem - kalloc1k_page_base(mem)) / KALLOC_1K_CHUNK_SIZE);
}

static inline uint32_t kalloc1k_chunk_mask(uint32_t index) {
    return (uint32_t)(1u << index);
}

static inline uint32_t kalloc1k_full_mask(void) {
    if (KALLOC_1K_CHUNKS_PER_PAGE >= 32)
        return 0xffffffffu;
    return (uint32_t)((1u << KALLOC_1K_CHUNKS_PER_PAGE) - 1u);
}

static void kfree1k_link_only(void *mem) {
    _free_list1k = page_list_prepend(_free_list1k, mem);
}

static void kalloc1k_unlink_page_chunks(ewokos_addr_t page_base) {
    page_list_t **current = &_free_list1k;

    while (*current != NULL) {
        if (kalloc1k_page_base(*current) == page_base) {
            *current = (*current)->next;
            continue;
        }
        current = &(*current)->next;
    }
}

static int kalloc1k_split_page(void) {
    char *page = kalloc_page();
    split_page_meta_t *meta;
    uint32_t i;

    if (page == NULL)
        return -1;

    meta = split_page_meta_alloc((ewokos_addr_t)page);
    if (meta == NULL) {
        kfree_page(page);
        return -1;
    }

    meta->free_mask = kalloc1k_full_mask();
    meta->free_count = KALLOC_1K_CHUNKS_PER_PAGE;
    for (i = 0; i < KALLOC_1K_CHUNKS_PER_PAGE; i++)
        kfree1k_link_only(page + i * KALLOC_1K_CHUNK_SIZE);

    return 0;
}

/*
 * page_list_prepend adds the given page to the beginning of the page list
 * and returns the address of the new page list.
 */
static page_list_t *page_list_prepend(page_list_t *page_list, char *page_address) {
    page_list_t *page = (page_list_t *) page_address;
    page->next = page_list;
    page->prev = NULL;
    if(page_list != NULL)
        page_list->prev = page;
    return page;
}

void kalloc_reset(void) {
    uint32_t i;

    _free_list_page = 0;
    _free_list1k = 0;
    _free_mem_size_page = 0;
    _page_busy = 0;
    _page_busy_max = 0;
    _split_page_meta_free = 0;
    for (i = 0; i < KALLOC_1K_META_BUCKETS; i++)
        _split_page_meta_buckets[i] = 0;
}

/* kalloc_append adds the given address range to the free list. */
uint32_t kalloc_append(ewokos_addr_t start, ewokos_addr_t end) {
    char *start_address = (char *) ALIGN_UP(start, PAGE_SIZE);
    char *end_address = (char *) ALIGN_DOWN(end, PAGE_SIZE);
    char *current_page = 0;
    uint32_t num = 0;

    /* Firmware maps may contain fragments smaller than the kernel page size.
     * After alignment such a range is empty (or inverted), not a page run. */
    if(start_address >= end_address)
        return 0;

    /* add each of the pages to the free list */
    for (current_page = start_address; current_page < end_address;
            current_page += PAGE_SIZE) {
        _free_list_page = page_list_prepend(_free_list_page, current_page);
        busy_mark(current_page, 0);
        num++;
    }
    _free_mem_size_page += (ewokos_addr_t)num * PAGE_SIZE;
    return num;
}

/* kalloc allocates and returns a single available page. and removed from free list*/
inline void* kalloc_page() {
    void *result = 0;
    if (_free_list_page != 0) {
        result = _free_list_page;
        _free_list_page = _free_list_page->next;
        if(_free_list_page != NULL)
            _free_list_page->prev = NULL;
        _free_mem_size_page -= PAGE_SIZE;
        busy_mark(result, 1);
    }
    return result;
}

/* kfree adds the given page to the _page free list. */
inline void kfree_page(void *page) {
    _free_list_page = page_list_prepend(_free_list_page, page);
    _free_mem_size_page += PAGE_SIZE;
    busy_mark(page, 0);
}

/* kalloc1k allocates 1k sized and aligned chuncks of memory. */
inline void* kalloc1k() {
    void *result = 0;
    split_page_meta_t *meta;
    uint32_t index;

    /*
     * If there is no 1K chunk available, split one page into PAGE_SIZE / 1K
     * chunks and track that page with lightweight metadata.
     */
    if (_free_list1k == 0 && kalloc1k_split_page() != 0)
        return NULL;

    if (_free_list1k != 0) {
        result = _free_list1k;
        _free_list1k = _free_list1k->next;

        meta = split_page_meta_find(kalloc1k_page_base(result));
        if (meta != NULL) {
            index = kalloc1k_chunk_index(result);
            if ((meta->free_mask & kalloc1k_chunk_mask(index)) != 0) {
                meta->free_mask &= ~kalloc1k_chunk_mask(index);
                if (meta->free_count > 0)
                    meta->free_count--;
            }
        }
    }
    return result;
}

/* kfree1k adds the given chunk to the 1k free list. */
inline void kfree1k(void *mem) {
    ewokos_addr_t page_base = kalloc1k_page_base(mem);
    uint32_t index = kalloc1k_chunk_index(mem);
    uint32_t mask = kalloc1k_chunk_mask(index);
    split_page_meta_t *meta = split_page_meta_find(page_base);

    if (meta == NULL) {
        kfree1k_link_only(mem);
        return;
    }

    if ((meta->free_mask & mask) != 0)
        return;

    meta->free_mask |= mask;
    meta->free_count++;
    kfree1k_link_only(mem);

    if (meta->free_count == KALLOC_1K_CHUNKS_PER_PAGE &&
            meta->free_mask == kalloc1k_full_mask()) {
        kalloc1k_unlink_page_chunks(page_base);
        split_page_meta_free(meta);
        kfree_page((void *)page_base);
    }
}

ewokos_addr_t get_free_mem_size(void) {
    /*
     * Keep the historical semantics: this reports the free capacity currently
     * available from the _page page list. 1K chunks split out for kalloc1k() are
     * intentionally not counted here, matching the previous implementation.
     */
    return _free_mem_size_page;
}

/* ------------------------------------------------------------------ */
/* physically-contiguous page allocation (DMA backing)                  */
/* ------------------------------------------------------------------ */

/*
 * Claim npages physically-contiguous free pages below phy_limit (absolute
 * physical address). Scans the occupancy bitmap for a free run, then unlinks
 * exactly those pages from the LIFO free list in one pass. Returns the kernel
 * direct-map VA of the run base, or NULL when no run exists. The caller owns
 * the pages (bitmap marked busy); release them with kfree_contig().
 */
void* kalloc_contig(uint32_t npages, ewokos_addr_t phy_limit) {
    if(npages == 0 || _page_busy == NULL || _pages_ref.refs == NULL)
        return NULL;
    ewokos_addr_t base = _pages_ref.phy_base;
    ewokos_addr_t top = base + (ewokos_addr_t)_page_busy_max * PAGE_SIZE;
    if(phy_limit < top)
        top = phy_limit;
    if(top <= base)
        return NULL;
    uint32_t limit_idx = (uint32_t)((top - base) / PAGE_SIZE);
    if(limit_idx < npages)
        return NULL;

    uint32_t run = 0;
    uint32_t start = 0;
    int found = 0;
    for(uint32_t i = 0; i < limit_idx; i++) {
        if(busy_test(i)) {
            run = 0;
        }
        else {
            run++;
            if(run == npages) {
                start = i - npages + 1;
                found = 1;
                break;
            }
        }
    }
    if(!found)
        return NULL;

    ewokos_addr_t base_pa = base + (ewokos_addr_t)start * PAGE_SIZE;
    char* base_va = (char*)P2V(base_pa);

    /* Unlink the run in O(npages): the bitmap guarantees every page in the
     * run is a free-list member, and the list is doubly linked, so each node
     * is spliced out directly via its prev/next without scanning the list.
     * (A singly linked list forced a full O(free-pages) walk per allocation,
     * which dominated boot time once DMA drew contiguous runs from general
     * RAM.) The taken nodes are chained for the caller; busy bits flip below. */
    page_list_t* taken = NULL;
    for(uint32_t j = 0; j < npages; j++) {
        page_list_t* node = (page_list_t*)(base_va + (ewokos_addr_t)j * PAGE_SIZE);
        if(node->prev != NULL)
            node->prev->next = node->next;
        else
            _free_list_page = node->next;
        if(node->next != NULL)
            node->next->prev = node->prev;
        node->next = taken;
        node->prev = NULL;
        taken = node;
    }
    for(uint32_t j = 0; j < npages; j++)
        busy_set(start + j);
    _free_mem_size_page -= (ewokos_addr_t)npages * PAGE_SIZE;
    return base_va;
}

/* Return a contiguous run claimed by kalloc_contig() to the page allocator.
 * Cache-attribute maintenance is the DMA layer's job (see dma.c). */
void kfree_contig(void* base, uint32_t npages) {
    if(base == NULL)
        return;
    char* p = (char*)base;
    for(uint32_t j = 0; j < npages; j++)
        kfree_page(p + (ewokos_addr_t)j * PAGE_SIZE);
}

/* Pop a free page whose physical address lies outside [lo, hi) AND inside
 * the refcounted allocable span (page_move needs both ends tracked). Pages
 * that do not qualify are stashed and pushed back afterwards. Bounded scan:
 * if the first 64 pops are all unusable, give up (caller tries another run). */
static void* kalloc_page_outside(ewokos_addr_t lo, ewokos_addr_t hi) {
    void* stash[64];
    uint32_t ns = 0;
    void* ret = NULL;
    ewokos_addr_t rbase = _pages_ref.phy_base;
    ewokos_addr_t rtop = rbase + (ewokos_addr_t)_pages_ref.max * PAGE_SIZE;
    for(uint32_t t = 0; t < 64; t++) {
        void* p = kalloc_page();
        if(p == NULL)
            break;
        ewokos_addr_t pa = V2P((ewokos_addr_t)p);
        if((pa < lo || pa >= hi) && pa >= rbase && pa < rtop) {
            ret = p;
            break;
        }
        stash[ns++] = p;
    }
    for(uint32_t i = 0; i < ns; i++)
        kfree_page(stash[i]);
    return ret;
}

/*
 * Contiguous allocation with compaction fallback. First try a direct free
 * run; if none exists, slide a npages-wide window over the range and evacuate
 * every occupied page that is a movable user leaf (refcount > 0 and not a
 * live DMA allocation / pinned) via page_move(), then claim the freed run.
 * Windows holding unmovable pages (page tables, slab, kernel structures,
 * DMA-owned storage) are skipped. Caller must hold the kernel lock with the
 * page quiescent (page_move contract). Returns the run base VA or NULL.
 */
void* kalloc_contig_compact(uint32_t npages, ewokos_addr_t phy_limit) {
    void* p = kalloc_contig(npages, phy_limit);
    if(p != NULL)
        return p;
    if(npages == 0 || _page_busy == NULL || _pages_ref.refs == NULL)
        return NULL;
    ewokos_addr_t base = _pages_ref.phy_base;
    ewokos_addr_t top = base + (ewokos_addr_t)_page_busy_max * PAGE_SIZE;
    if(phy_limit < top)
        top = phy_limit;
    if(top <= base)
        return NULL;
    uint32_t limit_idx = (uint32_t)((top - base) / PAGE_SIZE);
    if(limit_idx < npages)
        return NULL;

    for(uint32_t start = 0; start + npages <= limit_idx; start++) {
        int ok = 1;
        for(uint32_t j = 0; j < npages; j++) {
            uint32_t i = start + j;
            if(!busy_test(i))
                continue;
            ewokos_addr_t pa = base + (ewokos_addr_t)i * PAGE_SIZE;
            if(_pages_ref.refs[i] == 0 || !dma_page_movable(pa)) {
                ok = 0;
                break;
            }
        }
        if(!ok)
            continue;

        ewokos_addr_t win_lo = base + (ewokos_addr_t)start * PAGE_SIZE;
        ewokos_addr_t win_hi = win_lo + (ewokos_addr_t)npages * PAGE_SIZE;
        int fail = 0;
        for(uint32_t j = 0; j < npages; j++) {
            uint32_t i = start + j;
            if(!busy_test(i))
                continue;
            ewokos_addr_t pa = base + (ewokos_addr_t)i * PAGE_SIZE;
            void* pb = kalloc_page_outside(win_lo, win_hi);
            if(pb == NULL) {
                fail = 1;
                break;
            }
            if(page_move(pa, V2P((ewokos_addr_t)pb)) < 0) {
                kfree_page(pb);
                fail = 1;
                break;
            }
        }
        if(fail)
            continue;
        p = kalloc_contig(npages, phy_limit);
        if(p != NULL)
            return p;
    }
    return NULL;
}
