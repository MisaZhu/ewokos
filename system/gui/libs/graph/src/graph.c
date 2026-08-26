#include <graph/graph.h>
#include <g2dclient/g2dclient.h>
#include <ewoksys/dma.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#ifdef __cplusplus 
extern "C" { 
#endif

inline uint32_t argb(uint32_t a, uint32_t r, uint32_t g, uint32_t b) {
    return a << 24 | r << 16 | g << 8 | b;
}

inline uint8_t color_a(uint32_t c) {
    return (c >> 24) & 0xff;
}

inline uint8_t color_r(uint32_t c) {
    return (c >> 16) & 0xff;
}

inline uint8_t color_g(uint32_t c) {
    return (c >> 8) & 0xff;
}

inline uint8_t color_b(uint32_t c) {
    return c & 0xff;
}

inline uint32_t color_gray(uint32_t oc) {
    uint8_t oa = (oc >> 24) & 0xff;
    uint8_t or = (oc >> 16) & 0xff;
    uint8_t og = (oc >> 8)  & 0xff;
    uint8_t ob = oc & 0xff;
    or = (or + og + ob) / 3;
    return argb(oa, or, or, or);
}

inline uint32_t color_reverse(uint32_t oc) {
    uint8_t oa = (oc >> 24) & 0xff;
    uint8_t or = 0xff - ((oc >> 16) & 0xff);
    uint8_t og = 0xff - ((oc >> 8)  & 0xff);
    uint8_t ob = 0xff - (oc & 0xff);
    return argb(oa, or, og, ob);
}

inline uint32_t color_reverse_rgb(uint32_t oc) {
    uint8_t oa = (oc >> 24) & 0xff;
    uint8_t or = ((oc) & 0xff);
    uint8_t og = ((oc >> 8)  & 0xff);
    uint8_t ob = ((oc >> 16)  & 0xff);
    return argb(oa, or, og, ob);
}

static void* aligned_malloc(uint32_t size, uint32_t alignment) {
    // Check if alignment is a power of 2
    if ((alignment & (alignment - 1)) != 0) {
        return NULL; // Alignment must be a power of 2
    }
    // Calculate extra space needed (alignment offset + storing original pointer)
    uint32_t extra = alignment - 1 + sizeof(void*);
    void* raw_ptr = malloc(size + extra);
    if (!raw_ptr) return NULL;
    // Calculate aligned address
    ewokos_addr_t aligned_addr = (ewokos_addr_t)raw_ptr + sizeof(void*);
    aligned_addr = (aligned_addr + alignment - 1) & ~(alignment - 1);
    // Save original pointer for free
    *((void**)aligned_addr - 1) = raw_ptr;
    return (void*)aligned_addr;
}

// Corresponding free function
static void aligned_free(void* ptr) {
    if (ptr) {
        // Get original pointer and free
        void* raw_ptr = *((void**)ptr - 1);
        free(raw_ptr);
    }
}

inline void graph_init(graph_t* g, const uint32_t* buffer, int32_t w, int32_t h) {
    if(w <= 0 || h <= 0 || g == NULL)
        return;

    memset(g, 0, sizeof(graph_t));
    g->w = w;
    g->h = h;
    g->shm_id = -1;
    if(buffer != NULL) {
        g->buffer = (uint32_t*)buffer;
        g->need_free = false;
    }
    else {
        g->buffer = (uint32_t*)aligned_malloc(w*h*4, 32);
        g->need_free = true;
    }
    memset(&g->clip, 0, sizeof(grect_t));
}

void graph_set_clip(graph_t* g, int x, int y, int w, int h) {
    grect_t r = {x, y, w, h};
    g->clip.x = 0;
    g->clip.y = 0;
    g->clip.w = g->w;
    g->clip.h = g->h;
    grect_insect(&r, &g->clip);
}

void graph_unset_clip(graph_t* g) {
    memset(&g->clip, 0, sizeof(grect_t));
}

graph_t* graph_new(uint32_t* buffer, int32_t w, int32_t h) {
    if(w <= 0 || h <= 0)
        return NULL;

    graph_t* ret = (graph_t*)aligned_malloc(sizeof(graph_t), 32);
    if(ret != NULL)
        graph_init(ret, buffer, w, h);
    return ret;
}

graph_t* graph_new_shm(int32_t w, int32_t h) {
    graph_t* ret;
    uint32_t* pixels = NULL;
    int shm_id = -1;

    if(w <= 0 || h <= 0)
        return NULL;

    ret = (graph_t*)aligned_malloc(sizeof(graph_t), 32);
    if(ret == NULL)
        return NULL;

    /* the pixel buffer IS a keyed shm canvas shared with /dev/g2d,
       the id is kept in the graph so g2d ops can use it directly */
    if(g2d_shm_alloc((uint32_t)w * (uint32_t)h * sizeof(uint32_t),
                &shm_id, &pixels) != 0 || shm_id <= 0) {
        if(pixels != NULL)
            shmdt(pixels);
        aligned_free(ret);
        return NULL;
    }

    graph_init(ret, pixels, w, h);
    ret->shm_id = shm_id;
    ret->need_free = true; /* graph_free owns the shm canvas */
    return ret;
}

graph_t* graph_new_dma(int32_t w, int32_t h) {
    graph_t* ret;
    ewokos_addr_t addr;
    uint32_t size;

    if(w <= 0 || h <= 0)
        return NULL;

    ret = (graph_t*)aligned_malloc(sizeof(graph_t), 32);
    if(ret == NULL)
        return NULL;

    /* the pixel buffer comes from the sys_dma pool; dma_alloc() maps it
       into this process in the sys_dma v window, so the returned address
       is a usable pointer here and is carried to /dev/g2d as-is (g2dd
       mem-maps the same address on attach) */
    size = (uint32_t)w * (uint32_t)h * sizeof(uint32_t);
    addr = dma_alloc(0, size);
    if(addr == 0) {
        aligned_free(ret);
        return NULL;
    }

    graph_init(ret, (const uint32_t*)(uintptr_t)addr, w, h);
    ret->dma = true;
    ret->need_free = true; /* graph_free owns the dma allocation */
    return ret;
}

graph_t* graph_dup(graph_t* g) {
    if(g == NULL || g->buffer == NULL)
        return NULL;

    /* an shm-backed graph dups into its own shm canvas so the copy
       stays usable by g2d ops, zero copy like the source */
    graph_t* ret;
    if(g->shm_id > 0) {
        ret = graph_new_shm(g->w, g->h);
        if(ret == NULL)
            return NULL;
    }
    else {
        ret = graph_new(NULL, g->w, g->h);
        if(ret == NULL)
            return NULL;
    }
    /* graph_init/graph_new_shm can leave buffer NULL if the pixel
       allocation fails */
    if(ret->buffer == NULL) {
        graph_free(ret);
        return NULL;
    }
    memcpy(ret->buffer, g->buffer, g->w*g->h*4);
    return ret;
}

void graph_free(graph_t* g) {
    if(g == NULL)
        return;

    if(g->need_free) {
        if(g->shm_id > 0) {
            if(g->buffer != NULL)
                shmdt(g->buffer);
        }
        else if(g->dma) {
            if(g->buffer != NULL)
                dma_free(0, (ewokos_addr_t)(uintptr_t)g->buffer);
        }
        else if(g->buffer != NULL)
            aligned_free(g->buffer);
    }
    aligned_free(g);
}

void graph_clear(graph_t* g, uint32_t color) {
    if(g == NULL)
        return;
    if(g->w == 0 || g->h == 0)
        return;
    int32_t i = 0;
    int32_t sz = g->w * 4;
    while(i<g->w) {
        g->buffer[i] = color;
        ++i;
    }
    char* p = (char*)g->buffer;
    for(i=1; i<g->h; ++i) {
        memcpy(p+(i*sz), p, sz);
    }
}

void graph_reverse(graph_t* g) {
    if(g == NULL)
        return;
    for(int32_t i=0; i < g->w*g->h; i++) {
        g->buffer[i] = color_reverse(g->buffer[i]);
    }
}

void graph_reverse_rgb(graph_t* g) {
    if(g == NULL)
        return;
    for(int32_t i=0; i < g->w*g->h; i++) {
        g->buffer[i] = color_reverse_rgb(g->buffer[i]);
    }
}

void graph_gray(graph_t* g) {
    if(g == NULL)
        return;
    for(int32_t i=0; i < g->w*g->h; i++) {
        g->buffer[i] = color_gray(g->buffer[i]);
    }
}

#ifdef __cplusplus
}
#endif
