#include <mm/kalloc.h>
#include <kernel/kernel.h>
#include <mm/mmu.h>
#include <kernel/system.h>
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
    return page;
}

void kalloc_reset(void) {
    uint32_t i;

    _free_list_page = 0;
    _free_list1k = 0;
    _free_mem_size_page = 0;
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

    /* add each of the pages to the free list */
    for (current_page = start_address; current_page != end_address;
            current_page += PAGE_SIZE) {
        _free_list_page = page_list_prepend(_free_list_page, current_page);
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
        _free_mem_size_page -= PAGE_SIZE;
    }
    return result;
}

/* kfree adds the given page to the _page free list. */
inline void kfree_page(void *page) {
    _free_list_page = page_list_prepend(_free_list_page, page);
    _free_mem_size_page += PAGE_SIZE;
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
