#include <ewoksys/vdevice.h>
#include <ewoksys/utf8unicode.h>
#include <ewoksys/mstr.h>
#include <font/font.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FACE_PIXEL_DENT   64

/* Integer-keyed open-addressing glyph cache. Replaces the snprintf-keyed
 * hashmap: a hit costs a few integer ops and returns the cached slot
 * pointer directly (no per-glyph-per-frame FT_GlyphSlotRec copy, no key
 * formatting). Stored in font_t.cache (map_t is void*), so the font_t
 * layout is unchanged. */
#define GLYPH_CACHE_SLOTS 4096u   /* power of two */
#define GLYPH_CACHE_MAX   3072u   /* clear-all threshold, keeps probes short */

typedef struct {
    uint64_t key;                 /* (size << 32) | c, 0 = empty slot */
    FT_GlyphSlot slot;
} glyph_entry_t;

typedef struct {
    uint32_t count;
    glyph_entry_t entries[GLYPH_CACHE_SLOTS];
} glyph_cache_t;

static inline uint64_t glyph_key(uint32_t size, uint32_t c) {
    uint64_t key = ((uint64_t)size << 32) | c;
    return key == 0 ? (1ULL << 63) : key;
}

static inline uint32_t glyph_hash(uint64_t key) {
    /* splitmix64 finalizer: avalanches the sparse size:codepoint keys */
    key ^= key >> 30;
    key *= 0xbf58476d1ce4e5b9ULL;
    key ^= key >> 27;
    key *= 0x94d049bb133111ebULL;
    key ^= key >> 31;
    return (uint32_t)key & (GLYPH_CACHE_SLOTS - 1);
}

static void glyph_cache_clear(glyph_cache_t* cache) {
    for(uint32_t i = 0; i < GLYPH_CACHE_SLOTS; i++) {
        glyph_entry_t* e = &cache->entries[i];
        if(e->slot != NULL) {
            if(e->slot->bitmap.buffer != NULL)
                free(e->slot->bitmap.buffer);
            if(e->slot->other != NULL)
                free(e->slot->other);
            free(e->slot);
        }
        e->key = 0;
        e->slot = NULL;
    }
    cache->count = 0;
}

font_t* font_new(const char* name, bool safe) {
    if(font_init() != 0) 
        return NULL;

    font_t* font = (font_t*)calloc(sizeof(font_t), 1);
    strncpy(font->name, name, FONT_NAME_MAX-1);
    int id = font_load(name, "");
    if(id >= 0) {
        font->cache = calloc(1, sizeof(glyph_cache_t));
        font->id = id;
        return font;
    }
    if(safe) {
        id = font_load(DEFAULT_SYSTEM_FONT, "");
        if(id >= 0) {
            strncpy(font->name, DEFAULT_SYSTEM_FONT, FONT_NAME_MAX-1);
            font->id = id;
            font->cache = calloc(1, sizeof(glyph_cache_t));
            return font;
        }
    }
    free(font);
    return NULL;
}

int font_free(font_t* font) {
    if(font == NULL)
        return -1;
    font_close(font);
    free(font);
    return 0;
}

static void font_clear_cache(font_t* font) {
    if(font->cache != NULL) {
        glyph_cache_clear((glyph_cache_t*)font->cache);
    }
}

int font_close(font_t* font) {
    if(font == NULL)
        return -1;

    font_clear_cache(font);
    free(font->cache);
    font->cache = NULL;
    return 0;
}

static uint32_t bitmap_row_bytes(const FT_Bitmap* bmp) {
    if(bmp == NULL || bmp->pitch == 0)
        return 0;
    return (uint32_t)(bmp->pitch < 0 ? -bmp->pitch : bmp->pitch);
}

static void sanitize_slot_for_cache(FT_GlyphSlot slot) {
    if(slot == NULL)
        return;
    slot->library = NULL;
    slot->face = NULL;
    slot->next = NULL;
    slot->generic.data = NULL;
    slot->generic.finalizer = NULL;
    slot->outline.n_contours = 0;
    slot->outline.n_points = 0;
    slot->outline.points = NULL;
    slot->outline.tags = NULL;
    slot->outline.contours = NULL;
    slot->outline.flags = 0;
    slot->num_subglyphs = 0;
    slot->subglyphs = NULL;
    slot->control_data = NULL;
    slot->control_len = 0;
    slot->internal = NULL;
}

/* Deep-copy a freshly rendered slot into a standalone cache entry: the
 * backend (font_cs IPC / FreeType face) reuses its slot storage on the
 * next render, and outline/internal pointers must not be kept. */
static FT_GlyphSlot cache_slot_copy(FT_GlyphSlot slot) {
    FT_GlyphSlot p = (FT_GlyphSlot)malloc(sizeof(FT_GlyphSlotRec));
    if(p == NULL)
        return NULL;

    memcpy(p, slot, sizeof(FT_GlyphSlotRec));
    sanitize_slot_for_cache(p);
    uint32_t bmp_size = bitmap_row_bytes(&slot->bitmap) * slot->bitmap.rows;
    if(bmp_size > 0 && slot->bitmap.buffer != NULL) {
        p->bitmap.buffer = malloc(bmp_size);
        if(p->bitmap.buffer == NULL) {
            free(p);
            return NULL;
        }
        memcpy(p->bitmap.buffer, slot->bitmap.buffer, bmp_size);
    }
    else {
        /* empty glyph: never keep the backend's buffer pointer, it is
         * not ours to free on cache clear */
        p->bitmap.buffer = NULL;
    }
    if(slot->other != NULL) {
        p->other = malloc(sizeof(face_info_t));
        if(p->other == NULL) {
            if(p->bitmap.buffer != NULL)
                free(p->bitmap.buffer);
            free(p);
            return NULL;
        }
        memcpy(p->other, slot->other, sizeof(face_info_t));
    }
    return p;
}

/* Blit an 8-bit alpha glyph bitmap in one clipped span per row instead of
 * a bounds-checked call per pixel. Blend arithmetic is bit-identical to
 * graph_pixel_argb_raw (truncating /255, alpha-0 dest takes source RGB). */
static void glyph_blit(graph_t* g, int32_t x, int32_t y,
        const FT_Bitmap* bmp, uint32_t color) {
    if(g == NULL || bmp == NULL || bmp->buffer == NULL ||
            bmp->width == 0 || bmp->rows == 0)
        return;

    /* clip once for the whole glyph (same rule as graph_pixel_argb),
     * intersected with the canvas like graph_fill_cpu so a clip reaching
     * past the canvas edge can never index outside the buffer */
    int32_t cx0 = 0, cy0 = 0, cx1 = g->w, cy1 = g->h;
    if(g->clip.w != 0 && g->clip.h != 0) {
        cx0 = g->clip.x;
        cy0 = g->clip.y;
        cx1 = cx0 + g->clip.w;
        cy1 = cy0 + g->clip.h;
        if(cx0 < 0) cx0 = 0;
        if(cy0 < 0) cy0 = 0;
        if(cx1 > g->w) cx1 = g->w;
        if(cy1 > g->h) cy1 = g->h;
    }
    int32_t bw = (int32_t)bmp->width;
    int32_t bh = (int32_t)bmp->rows;
    int32_t sx0 = (x < cx0) ? cx0 - x : 0;
    int32_t sy0 = (y < cy0) ? cy0 - y : 0;
    int32_t sx1 = (x + bw > cx1) ? cx1 - x : bw;
    int32_t sy1 = (y + bh > cy1) ? cy1 - y : bh;
    if(sx0 >= sx1 || sy0 >= sy1)
        return;

    uint32_t row_bytes = bitmap_row_bytes(bmp);
    uint32_t ca = (color >> 24) & 0xff;
    uint32_t cr = (color >> 16) & 0xff;
    uint32_t cg = (color >> 8) & 0xff;
    uint32_t cb = color & 0xff;
    uint32_t solid = 0xff000000 | (cr << 16) | (cg << 8) | cb;
    uint32_t rgb = (cr << 16) | (cg << 8) | cb;
    int32_t n = sx1 - sx0;

    for(int32_t j = sy0; j < sy1; j++) {
        const uint8_t* src = bmp->buffer + (uint32_t)j * row_bytes + sx0;
        uint32_t* dst = g->buffer + (y + j) * g->w + (x + sx0);
        for(int32_t i = 0; i < n; i++) {
            uint32_t a = ca & src[i];
            if(a == 0)
                continue;
            if(a == 0xff) {
                dst[i] = solid;
                continue;
            }
            uint32_t oc = dst[i];
            uint32_t oa = oc >> 24;
            if(oa == 0) {
                dst[i] = (a << 24) | rgb;
                continue;
            }
            uint32_t inv_a = 255 - a;
            uint32_t na = oa + (255 - oa) * a / 255;
            uint32_t nr = (cr * a + ((oc >> 16) & 0xff) * inv_a) / 255;
            uint32_t ng = (cg * a + ((oc >> 8) & 0xff) * inv_a) / 255;
            uint32_t nb = (cb * a + (oc & 0xff) * inv_a) / 255;
            dst[i] = (na << 24) | (nr << 16) | (ng << 8) | nb;
        }
    }
}

FT_GlyphSlot font_init_glyph(FT_GlyphSlot glyph) {
    memset(glyph, 0, sizeof(FT_GlyphSlotRec));
    return glyph;
}

int  font_get_height(font_t* font, uint32_t size) {
    face_info_t face;
    if(font_get_face(font, size, &face) != 0)
        return 0;
    return face.height/FACE_PIXEL_DENT;
}

int  font_get_width(font_t* font, uint32_t size) {
    face_info_t face;
    if(font_get_face(font, size, &face) != 0)
        return 0;
    return face.width/FACE_PIXEL_DENT;
}

/* Cache hit returns the cached slot pointer (no copy). Miss renders into
 * scratch, caches a sanitized deep copy and returns that (scratch itself
 * if the copy could not be allocated). NULL on failure. */
static FT_GlyphSlot font_get_glyph(font_t* font, uint32_t size, uint32_t c,
        FT_GlyphSlotRec* scratch) {
    if(font == NULL)
        return NULL;

    glyph_cache_t* cache = (glyph_cache_t*)font->cache;
    uint64_t key = glyph_key(size, c);
    if(cache != NULL) {
        uint32_t i = glyph_hash(key);
        for(uint32_t n = 0; n < GLYPH_CACHE_SLOTS; n++) {
            glyph_entry_t* e = &cache->entries[i];
            if(e->key == key)
                return e->slot;
            if(e->key == 0)
                break;
            i = (i + 1) & (GLYPH_CACHE_SLOTS - 1);
        }
    }

    memset(scratch, 0, sizeof(FT_GlyphSlotRec));
    if(font_get_glyph_info(font, size, c, scratch) != 0)
        return NULL;

    if(cache != NULL) {
        FT_GlyphSlot p = cache_slot_copy(scratch);
        if(p != NULL) {
            if(cache->count >= GLYPH_CACHE_MAX)
                glyph_cache_clear(cache);
            uint32_t i = glyph_hash(key);
            while(cache->entries[i].key != 0)
                i = (i + 1) & (GLYPH_CACHE_SLOTS - 1);
            cache->entries[i].key = key;
            cache->entries[i].slot = p;
            cache->count++;
            return p;
        }
    }
    return scratch;
}

/* Advance of one rendered glyph, identical for measurement and drawing:
 * horiAdvance, tab = 2x the tab bitmap width (size when empty). Keeping
 * one rule makes font_text_size agree with the graph_draw_text_font pen
 * step, so align boxes sized from the measurement fit the drawn text. */
static uint32_t glyph_advance(FT_GlyphSlot slot, uint32_t size, uint32_t c) {
    if(c == '\t') {
        uint32_t w = slot->bitmap.width*2;
        return w == 0 ? size : w;
    }
    uint32_t w = slot->metrics.horiAdvance / FACE_PIXEL_DENT;
    return w == 0 ? size/2 : w;
}

void font_char_size(uint32_t c, font_t* font, uint32_t size, uint32_t *w, uint32_t* h) {
    uint32_t cw = size/2;
    uint32_t ch = size;

    FT_GlyphSlotRec scratch;
    FT_GlyphSlot slot = font_get_glyph(font, size, c, &scratch);
    if(slot != NULL) {
        cw = glyph_advance(slot, size, c);
        face_info_t* faceinfo = (face_info_t*)slot->other;
        if(faceinfo != NULL) {
            ch = faceinfo->height/FACE_PIXEL_DENT;
            if(ch == 0) ch = size;
        }
    }
    if(w != NULL) *w = cw;
    if(h != NULL) *h = ch;
}

/* One glyph walk over a decoded run: advance width, line height and the
 * union of the real glyph ink (top/bot relative to the y handed to the
 * draw path, which blits at y - bitmap_top + ascender). ink is false
 * when no glyph metrics are available. Rides the glyph cache, so warm
 * strings cost no IPC. */
static bool measure_run(const uint16_t* out, int n, font_t* font, uint32_t size,
        uint32_t* w, uint32_t* h, int32_t* top, int32_t* bot) {
    uint32_t tw = 0, th = 0;
    bool any = false;
    int32_t t = 0, b = 0;
    for(int i=0; i<n; i++) {
        uint32_t c = out[i];
        FT_GlyphSlotRec scratch;
        FT_GlyphSlot slot = font_get_glyph(font, size, c, &scratch);
        if(slot == NULL) {
            tw += size/2;
            if(th < size) th = size;
            continue;
        }
        tw += glyph_advance(slot, size, c);
        face_info_t* fi = (face_info_t*)slot->other;
        uint32_t ch = size;
        if(fi != NULL) {
            ch = fi->height/FACE_PIXEL_DENT;
            if(ch == 0) ch = size;
        }
        if(th < ch) th = ch;

        if(c == ' ' || c == '\t' || c == 0x3000)
            continue;
        if(slot->bitmap.rows == 0 || fi == NULL)
            continue;
        int32_t gt = fi->ascender/FACE_PIXEL_DENT - slot->bitmap_top;
        int32_t gb = gt + (int32_t)slot->bitmap.rows;
        if(!any || gt < t) t = gt;
        if(!any || gb > b) b = gb;
        any = true;
    }
    if(w != NULL) *w = tw;
    if(h != NULL) *h = th;
    if(any && top != NULL && bot != NULL) {
        *top = t;
        *bot = b;
    }
    return any;
}

static bool text_measure(const char* str, font_t* font, uint32_t size,
        uint32_t* w, uint32_t* h, int32_t* top, int32_t* bot) {
    if(w != NULL) *w = 0;
    if(h != NULL) *h = 0;
    if(str == NULL)
        return false;

    int len = strlen(str);
    uint16_t* out = (uint16_t*)malloc((len+1)*2);
    if(out == NULL)
        return false;
    int n = utf82unicode((uint8_t*)str, len, out);
    bool any = measure_run(out, n, font, size, w, h, top, bot);
    free(out);
    return any;
}

void font_text_size(const char* str,
        font_t* font, uint32_t size, uint32_t *w, uint32_t *h) {
    text_measure(str, font, size, w, h, NULL, NULL);
}

void graph_draw_unicode_font(graph_t* g, int32_t x, int32_t y, uint32_t c,
        font_t* font, uint32_t size, uint32_t color, uint32_t *w, uint32_t *h) {
    uint32_t dw = size/2;
    uint32_t dh = size;

    FT_GlyphSlotRec scratch;
    FT_GlyphSlot slot = font_get_glyph(font, size, c, &scratch);
    if(slot != NULL) {
        dw = glyph_advance(slot, size, c);
        face_info_t* faceinfo = (face_info_t*)slot->other;
        int32_t asc = 0;
        if(faceinfo != NULL) {
            dh = faceinfo->height/FACE_PIXEL_DENT;
            if(dh == 0) dh = size;
            asc = faceinfo->ascender/FACE_PIXEL_DENT;
        }
        if(c != '\t')
            glyph_blit(g, x + slot->bitmap_left, y - slot->bitmap_top + asc,
                    &slot->bitmap, color);
    }
    if(w != NULL) *w = dw;
    if(h != NULL) *h = dh;
}

void graph_draw_char_font_fixed(graph_t* g, int32_t x, int32_t y, uint32_t c,
        font_t* font, uint32_t size, uint32_t color, uint32_t w, uint32_t h) {
    (void)h;
    FT_GlyphSlotRec scratch;
    FT_GlyphSlot slot = font_get_glyph(font, size, c, &scratch);
    if(slot == NULL)
        return;
    face_info_t* faceinfo = (face_info_t*)slot->other;
    int32_t asc = faceinfo != NULL ? faceinfo->ascender/FACE_PIXEL_DENT : 0;

    /* centre the ink box [x+left, x+left+width] inside the cell: the
     * old (w - (width - left))/2 ignored the bearing on the left side */
    x += ((int)w - (int)slot->bitmap.width)/2 - slot->bitmap_left;
    y = y - slot->bitmap_top + asc;

    glyph_blit(g, x, y, &slot->bitmap, color);
}

static void draw_unicode_run(graph_t* g, int32_t x, int32_t y,
        const uint16_t* out, int n, font_t* font, uint32_t size, uint32_t color) {
    for(int i=0;i <n; i++) {
        uint32_t w = 0;
        graph_draw_unicode_font(g, x, y, out[i], font, size, color, &w, NULL);
        x += w;
    }
}

void graph_draw_text_font(graph_t* g, int32_t x, int32_t y, const char* str,
        font_t* font, uint32_t size, uint32_t color) {
    if(g == NULL || str == NULL || str[0] == 0)
        return;

    int len = strlen(str);
    uint16_t* out = (uint16_t*)malloc((len+1)*2);
    if(out == NULL)
        return;
    int n = utf82unicode((uint8_t*)str, len, out);
    draw_unicode_run(g, x, y, out, n, font, size, color);
    free(out);
}

void graph_draw_text_font_align(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h,
        const char* str, font_t* font, uint32_t size, uint32_t color, uint32_t align) {
    if(g == NULL || str == NULL || str[0] == 0)
        return;

    int len = strlen(str);
    uint16_t* out = (uint16_t*)malloc((len+1)*2);
    if(out == NULL)
        return;
    int n = utf82unicode((uint8_t*)str, len, out);

    /* one decode + one glyph walk for width, line height and ink union;
     * the draw below reuses the same decoded run */
    uint32_t fw = 0, fh = 0;
    int32_t top = 0, bot = 0;
    bool ink = measure_run(out, n, font, size, &fw, &fh, &top, &bot);

    /* horizontal: advance-width box; LEFT/NONE keep x */
    if((align & FONT_ALIGN_H_CENTER) != 0)
        x = x + (w - (int32_t)fw)/2;
    else if((align & FONT_ALIGN_RIGHT) != 0)
        x = x + w - (int32_t)fw;

    /* vertical: align the real glyph ink, not the line box — CJK faces
     * (system-cn: +1.16em/-0.29em) have a tall asymmetric line box, so
     * line-box centring lands visibly low. NONE keeps the legacy y. */
    if((align & (FONT_ALIGN_V_CENTER|FONT_ALIGN_TOP|FONT_ALIGN_BOTTOM)) != 0) {
        if((align & FONT_ALIGN_V_CENTER) != 0) {
            if(ink) {
                y = y + (h - top - bot)/2;
            }
            else {
                /* no glyph metrics: ascender estimate, then line box */
                face_info_t face;
                if(font_get_face(font, size, &face) == 0 && face.ascender > 0) {
                    int32_t baseline = y + h/2 + ((int32_t)size*37 + 50)/100;
                    y = baseline - face.ascender/FACE_PIXEL_DENT;
                }
                else {
                    y = y + (h - (int32_t)fh)/2;
                }
            }
        }
        else if((align & FONT_ALIGN_BOTTOM) != 0) {
            /* ink bottom flush with the box bottom */
            y = ink ? (y + h - bot) : (y + h - (int32_t)fh);
        }
        else if(ink) {
            /* TOP: ink top flush with the box top */
            y = y - top;
        }
    }
    draw_unicode_run(g, x, y, out, n, font, size, color);
    free(out);
}

#ifdef __cplusplus
}
#endif
