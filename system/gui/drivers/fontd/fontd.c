#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <font/font.h>
#include <tinyjson/tinyjson.h>
#include <freetype/freetype.h>
#include "ewoksys/mstr.h"

#define NAME_LEN 64
#define FNAME_LEN 128

typedef struct {
    char name[NAME_LEN];
    char fname[FNAME_LEN];
    FT_Face face;
    uint8_t* mem;   /* whole-file image backing the face (FT_New_Memory_Face) */
    int refs;       /* live font_t handles; 0 = registered but unreferenced */
    uint64_t use;   /* LRU tick for evicting unreferenced slots */
    //uint32_t current_size;
} ttf_item_t;

#define TTF_MAX  256

static ttf_item_t _ttfs[TTF_MAX];
static uint64_t _use_clock = 0;

static FT_Library _library;
#define DO_FONT_CACHE 1
static map_t _font_cache = NULL;

static void font_clear_cache(void);

/* Transient faces are extracted per-document programs (e.g. ereader's
 * PDF embedded subsets dropped under /tmp/fonts). They stay registered
 * for reuse after the last handle closes, but are not installed fonts and
 * must not pollute the user-visible font list. */
static int font_is_transient(const ttf_item_t* item) {
    return strncmp(item->fname, "/tmp/", 5) == 0;
}

static void font_dev_init(void) {
    _font_cache = hashmap_new(0);

    FT_Init_FreeType(&_library);
    for(int i=0;i < TTF_MAX; i++)
        memset(&_ttfs[i], 0, sizeof(ttf_item_t));
}

static void font_dev_quit(void) {
    for(int i=0;i < TTF_MAX; i++) {
        if(_ttfs[i].face != NULL)
            FT_Done_Face(_ttfs[i].face);
        if(_ttfs[i].mem != NULL)
            free(_ttfs[i].mem);
    }

    font_clear_cache();
}

static char* font_cmd(vdevice_t* dev, int from_pid, int argc, char** argv, void* p) {
    (void)dev;
    if(strcmp(argv[0], "list") == 0) {
        str_t* str = str_new("");
        for(int i=0; i<TTF_MAX; i++) {
            if(_ttfs[i].face != NULL && !font_is_transient(&_ttfs[i])) {
                str_add(str, _ttfs[i].name);
                str_add(str, ", ");
                str_add(str, _ttfs[i].fname);
                str_add(str, "\n");
            }
        }
        char* ret = str_detach(str);
        return ret;
    }
    return NULL;
}

/* Slot allocation + container whitelist + FT registration. Takes ownership
 * of mem on success (face backs onto it); the caller frees mem on failure. */
static int font_register_mem(const char* name, const char* fname,
        uint8_t* mem, size_t fsz) {
    int i;
    for(i=0;i < TTF_MAX; i++) {
        if(_ttfs[i].face == NULL)
            break;
    }
    if(i >= TTF_MAX) {
        /* table full: recycle the least recently used slot no client holds */
        uint64_t best = ~0ull;
        i = -1;
        for(int k=0; k<TTF_MAX; k++) {
            if(_ttfs[k].face != NULL && _ttfs[k].refs == 0 && _ttfs[k].use < best) {
                best = _ttfs[k].use;
                i = k;
            }
        }
        if(i < 0)
            return -1;
        FT_Done_Face(_ttfs[i].face);
        free(_ttfs[i].mem);
        _ttfs[i].face = NULL;
        _ttfs[i].mem = NULL;
        _ttfs[i].name[0] = 0;
        _ttfs[i].fname[0] = 0;
        font_clear_cache();
    }

    /*
     * Accept the containers FreeType's outline drivers parse: SFNT
     * (TrueType/OpenType), bare CFF (PDF FontFile3) and Type1 (PDF
     * FontFile, PFB or PFA). Bitmap-only legacy formats stay excluded.
     */
    uint8_t* header = mem;
    if(!((header[0] == 0x00 && header[1] == 0x01 && header[2] == 0x00 && header[3] == 0x00) ||
            memcmp(header, "OTTO", 4) == 0 ||
            memcmp(header, "true", 4) == 0 ||
            memcmp(header, "ttcf", 4) == 0 ||
            (header[0] == 0x01 && header[1] == 0x00 && header[2] == 0x04) ||
            (header[0] == 0x80 && header[1] == 0x01) ||
            (fsz >= 14 && memcmp(header, "%!PS-AdobeFont", 14) == 0) ||
            (fsz >= 11 && memcmp(header, "%!FontType1", 11) == 0))) {
        return -1;
    }

    int err = FT_New_Memory_Face(_library, mem, (FT_Long)fsz, 0, &_ttfs[i].face);
    if(err != 0) {
        _ttfs[i].face = NULL;
        return -1;
    }
    _ttfs[i].mem = mem;
    _ttfs[i].refs = 0;
    _ttfs[i].use = ++_use_clock;
    strncpy(_ttfs[i].name, name, NAME_LEN-1);
    strncpy(_ttfs[i].fname, fname, FNAME_LEN-1);
    return i;
}

int font_open(const char* name, const char* fname) {
    for(int i=0; i<TTF_MAX; i++) {
        if(_ttfs[i].face != NULL &&
                strcmp(name, _ttfs[i].name) == 0) {
            /* empty fname = handle lookup (font_new): take a reference */
            if(fname[0] == 0)
                _ttfs[i].refs++;
            _ttfs[i].use = ++_use_clock;
            return i;
        }
    }
    if(fname[0] == 0)
        return -1;
    for(int i=0; i<TTF_MAX; i++) {
        if(_ttfs[i].face != NULL &&
                strcmp(fname, _ttfs[i].fname) == 0) {
            /* same program under a new name: reuse the face, re-tag the name */
            strncpy(_ttfs[i].name, name, NAME_LEN-1);
            _ttfs[i].use = ++_use_clock;
            return i;
        }
    }

    int fd = open(fname, O_RDONLY);
    if(fd < 0) {
        return -1;
    }
    /* Read the whole container in bounded chunks and hand FreeType a memory
     * image. The ANSI stdio stream backend would issue one IPC read per
     * fread/fgetc on this OS, a device-only parse path that can mis-read
     * subset fonts; a memory face parses exactly like the host does. */
    off_t fsz = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    if(fsz <= 4 || fsz > 64*1024*1024) {
        close(fd);
        return -1;
    }
    uint8_t* mem = (uint8_t*)malloc((size_t)fsz);
    if(mem == NULL) {
        close(fd);
        return -1;
    }
    size_t got = 0;
    while(got < (size_t)fsz) {
        size_t want = (size_t)fsz - got;
        if(want > 32768) want = 32768;
        ssize_t r = read(fd, mem + got, want);
        if(r <= 0) break;
        got += (size_t)r;
    }
    close(fd);
    if(got != (size_t)fsz) {
        free(mem);
        return -1;
    }
    int i = font_register_mem(name, fname, mem, (size_t)fsz);
    if(i < 0) {
        free(mem);
        return -1;
    }
    return i;
}

static int font_dev_load(proto_t* in, proto_t* ret) {
    const char* name = proto_read_str(in);
    const char* fname = proto_read_str(in);

    if(name == NULL || fname == NULL) {
        PF->init(ret)->addi(ret, -1);
        return 0;
    }

    int res = font_open(name, fname);
    PF->init(ret)->addi(ret, res);
    return 0;
}

static int free_cache(map_t map, const char* key, any_t data, any_t arg) {
    (void)map;
    (void)key;
    (void)arg;
    FT_GlyphSlot slot = (FT_GlyphSlot)data;
    if(slot == NULL)
        return MAP_OK;

    if(slot->bitmap.buffer != NULL)
        free(slot->bitmap.buffer);
    free(slot);
    return MAP_OK;
}

static void font_clear_cache(void) {
    if(_font_cache != NULL) {
        map_t old_cache = _font_cache;
        _font_cache = hashmap_new(0);
        hashmap_iterate(old_cache, free_cache, NULL);
        hashmap_free(old_cache);
    }
}

static inline const char* hash_key(int32_t findex, uint32_t c, uint32_t size) {
    static char key[32] = {0};
    snprintf(key, sizeof(key), "%d:%u:%x", findex, size, c);
    return key;
}

static FT_Int _load_mode = FT_LOAD_TARGET_NORMAL;
static FT_Int _hinting = FT_LOAD_NO_HINTING | FT_LOAD_NO_AUTOHINT;

static uint32_t bitmap_row_bytes(const FT_Bitmap* bmp) {
    if(bmp == NULL || bmp->pitch == 0)
        return 0;
    return (uint32_t)(bmp->pitch < 0 ? -bmp->pitch : bmp->pitch);
}

static void sanitize_slot_for_ipc(FT_GlyphSlot slot) {
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
    slot->other = NULL;
    slot->internal = NULL;
}

static int expand_mono_bitmap(FT_GlyphSlot slot) {
    if(slot == NULL || slot->bitmap.buffer == NULL ||
            slot->bitmap.rows == 0 || slot->bitmap.width == 0) {
        return 0;
    }

    uint32_t src_row_bytes = bitmap_row_bytes(&slot->bitmap);
    uint32_t tight_bytes = (uint32_t)slot->bitmap.width;
    uint32_t total = tight_bytes * slot->bitmap.rows;
    uint8_t* expanded = (uint8_t*)malloc(total);
    if(expanded == NULL)
        return -1;

    const uint8_t* src = slot->bitmap.buffer;
    if(slot->bitmap.pitch < 0)
        src += (slot->bitmap.rows - 1) * src_row_bytes;

    for(uint32_t y = 0; y < slot->bitmap.rows; y++) {
        for(uint32_t x = 0; x < slot->bitmap.width; x++) {
            uint8_t byte = src[x >> 3];
            uint8_t bit = 0x80 >> (x & 7);
            expanded[y * tight_bytes + x] = (byte & bit) ? 0xff : 0x00;
        }
        src += slot->bitmap.pitch;
    }

    slot->bitmap.buffer = expanded;
    slot->bitmap.pitch = (int)tight_bytes;
    slot->bitmap.pixel_mode = FT_PIXEL_MODE_GRAY;
    slot->bitmap.num_grays = 256;
    return 0;
}

static int normalize_slot_bitmap(FT_GlyphSlot slot) {
    if(slot == NULL || slot->bitmap.buffer == NULL ||
            slot->bitmap.rows == 0 || slot->bitmap.width == 0) {
        return 0;
    }

    if(slot->bitmap.pixel_mode == FT_PIXEL_MODE_MONO)
        return expand_mono_bitmap(slot);

    uint32_t row_bytes = bitmap_row_bytes(&slot->bitmap);
    uint32_t tight_bytes = (uint32_t)slot->bitmap.width;
    if(row_bytes == 0 || tight_bytes == 0)
        return 0;
    if(row_bytes == tight_bytes && slot->bitmap.pitch > 0)
        return 0;

    uint32_t total = tight_bytes * slot->bitmap.rows;
    uint8_t* normalized = (uint8_t*)malloc(total);
    if(normalized == NULL)
        return -1;

    const uint8_t* src = slot->bitmap.buffer;
    if(slot->bitmap.pitch < 0)
        src += (slot->bitmap.rows - 1) * row_bytes;

    for(uint32_t y = 0; y < slot->bitmap.rows; y++) {
        memcpy(normalized + y * tight_bytes, src, tight_bytes);
        src += slot->bitmap.pitch;
    }

    slot->bitmap.buffer = normalized;
    slot->bitmap.pitch = (int)tight_bytes;
    return 0;
}

static int load_glyph_rendered(FT_Face face, FT_UInt glyph_index) {
    /*
     * Keep the default path fully unhindered. FreeType's hinted/auto-hint
     * paths have been observed crashing on target, so only opt into them when
     * explicitly requested via `-h`.
     */
    int ret = 0;

    if(_hinting == FT_LOAD_FORCE_AUTOHINT) {
        FT_Int flags = FT_LOAD_DEFAULT | FT_LOAD_FORCE_AUTOHINT | _load_mode;
        ret = FT_Load_Glyph(face, glyph_index, flags);
        if(ret == 0) {
            ret = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
            if(ret == 0)
                return 0;
        }
    }

    FT_Int flags = FT_LOAD_DEFAULT | FT_LOAD_NO_HINTING | FT_LOAD_NO_AUTOHINT;
    ret = FT_Load_Glyph(face, glyph_index, flags);
    if(ret == 0) {
        ret = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        if(ret == 0)
            return 0;
    }

    flags = FT_LOAD_DEFAULT | FT_LOAD_NO_HINTING | FT_LOAD_NO_AUTOHINT | FT_LOAD_TARGET_MONO;
    ret = FT_Load_Glyph(face, glyph_index, flags);
    if(ret != 0)
        return ret;
    ret = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_MONO);
    if(ret != 0)
        return ret;
    return 0;
}

static int font_fetch_cache(int32_t findex, uint32_t size, uint32_t c, FT_GlyphSlot slot) {
    if(_font_cache == NULL) {
        return -1;
    }

    FT_GlyphSlot p;
    if(hashmap_get(_font_cache, hash_key(findex, c, size), (void**)&p) != 0)
        return -1;
    if(p == NULL)
        return -1;
    memcpy(slot, p, sizeof(FT_GlyphSlotRec));
    return 0;
}

#define MAX_FONT_CACHE 4096

static void font_cache(int32_t findex, uint32_t size, uint32_t c, FT_GlyphSlot slot) {
#if DO_FONT_CACHE
    if(_font_cache == NULL || slot == NULL) {
        return;
    }

    FT_GlyphSlot p = (FT_GlyphSlot)malloc(sizeof(FT_GlyphSlotRec));
    if(p == NULL)
        return;

    if(hashmap_length(_font_cache) >= MAX_FONT_CACHE) {
        font_clear_cache();
    }
    memcpy(p, slot, sizeof(FT_GlyphSlotRec));
    sanitize_slot_for_ipc(p);
    uint32_t row_bytes = bitmap_row_bytes(&slot->bitmap);
    uint32_t sz = row_bytes * slot->bitmap.rows;
    if(sz > 0 && slot->bitmap.buffer != NULL) {
        p->bitmap.buffer = malloc(sz);
        if(p->bitmap.buffer == NULL) {
            free(p);
            return;
        }
        memcpy(p->bitmap.buffer, slot->bitmap.buffer, sz);
    }
    else {
        p->bitmap.buffer = NULL;
    }
    hashmap_put(_font_cache, hash_key(findex, c, size), p);
#endif
}

static int font_dev_get_glyph(proto_t* in, proto_t* ret) {
    int findex = proto_read_int(in);
    uint32_t size = (uint32_t)proto_read_int(in);
    uint32_t c = (uint32_t)proto_read_int(in);
    if(findex < 0 || findex >= TTF_MAX || size == 0 || _ttfs[findex].face == NULL) {
        PF->init(ret)->addi(ret, -1);
        return -1;
    }
    _ttfs[findex].use = ++_use_clock;

    FT_Face face = _ttfs[findex].face; 
    uint8_t* face_bitmap_buffer = NULL;
    int release_slot_bitmap = 0;
    //if(_ttfs[findex].current_size != size) {
        if(FT_Set_Pixel_Sizes(face, 0, size) != 0) {
            PF->init(ret)->addi(ret, -1);
            return -1;
        }
        //_ttfs[findex].current_size = size;
    //}
    FT_GlyphSlotRec slot;
    FT_UInt glyph_index = 0;
    int cache_hit = (font_fetch_cache(findex, size, c, &slot) == 0);
    if(!cache_hit) {
        glyph_index = FT_Get_Char_Index(face, c);
        if(glyph_index == 0) {
            memset(&slot, 0, sizeof(slot));
            if(c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                slot.metrics.horiAdvance = face->size->metrics.max_advance / 2;
            }
            else {
                slot.metrics.horiAdvance = face->size->metrics.max_advance;
            }
            if(slot.metrics.horiAdvance == 0)
                slot.metrics.horiAdvance = size * 32;
        }
        else {
            int glyph_err = load_glyph_rendered(face, glyph_index);
            if(glyph_err != 0) {
                PF->init(ret)->addi(ret, -1);
                return -1;
            }
            face_bitmap_buffer = face->glyph->bitmap.buffer;
            memcpy(&slot, face->glyph, sizeof(FT_GlyphSlotRec));
            glyph_err = normalize_slot_bitmap(&slot);
            if(glyph_err != 0) {
                PF->init(ret)->addi(ret, -1);
                return -1;
            }
            release_slot_bitmap = (slot.bitmap.buffer != NULL && slot.bitmap.buffer != face_bitmap_buffer);
            sanitize_slot_for_ipc(&slot);
        }
        font_cache(findex, size, c, &slot);
    }

    face_info_t faceinfo;
    faceinfo.ascender = face->size->metrics.ascender;
    faceinfo.descender = face->size->metrics.descender;
    faceinfo.height = face->size->metrics.ascender - face->size->metrics.descender;
    faceinfo.width = face->size->metrics.max_advance;
    uint32_t bmp_size = bitmap_row_bytes(&slot.bitmap) * slot.bitmap.rows;
    PF->init(ret)->addi(ret, 0);
    if(bmp_size > 0) {
        PF->add(ret, &slot, sizeof(FT_GlyphSlotRec))->
            add(ret, &faceinfo, sizeof(face_info_t))->
            add(ret, slot.bitmap.buffer, bmp_size);
    }
    else {
        PF->add(ret, &slot, sizeof(FT_GlyphSlotRec))->
            add(ret, &faceinfo, sizeof(face_info_t));
    }
    if(release_slot_bitmap) {
        free(slot.bitmap.buffer);
        slot.bitmap.buffer = NULL;
    }
    return 0;
}


static int font_dev_get_face(proto_t* in, proto_t* ret) {
    int i = proto_read_int(in);
    uint32_t size = (uint32_t)proto_read_int(in);
    if(i < 0 || i >= TTF_MAX || size == 0 || _ttfs[i].face == NULL) {
        PF->init(ret)->addi(ret, -1);
        return -1;
    }
    _ttfs[i].use = ++_use_clock;

    FT_Face face = _ttfs[i].face; 
    if(FT_Set_Pixel_Sizes(face, 0, size) != 0) {
        PF->init(ret)->addi(ret, -1);
        return -1;
    }

    face_info_t faceinfo;
    faceinfo.ascender = face->size->metrics.ascender;
    faceinfo.descender = face->size->metrics.descender;
    faceinfo.height = face->size->metrics.ascender - face->size->metrics.descender;
    faceinfo.width = face->size->metrics.max_advance;
    PF->init(ret)->addi(ret, 0)->add(ret, &faceinfo, sizeof(face_info_t));
    return 0;
}

static int font_dev_list(proto_t* in, proto_t* ret) {
    for(int i=0; i<TTF_MAX; i++) {
        if(_ttfs[i].face != NULL && !font_is_transient(&_ttfs[i])) {
            PF->adds(ret, _ttfs[i].name);
        }
    }
    return 0;
}

static int font_dev_free(proto_t* in, proto_t* ret) {
    int i = proto_read_int(in);
    if(i < 0 || i >= TTF_MAX || _ttfs[i].face == NULL) {
        PF->init(ret)->addi(ret, -1);
        return -1;
    }
    if(_ttfs[i].refs > 0)
        _ttfs[i].refs--;
    /* Keep the face registered when the last handle closes. Clients reopen
     * the same book/font repeatedly (ereader close->open); destroying here
     * lost every non-system face (only "system" survives, held by xwin), so
     * the next open fell back to the Latin system font and re-parsed all
     * embedded programs. Unreferenced slots are recycled by font_open's LRU
     * only when the table is actually full. */
    _ttfs[i].use = ++_use_clock;
    PF->init(ret)->addi(ret, 0);
    return 0;
}

static int font_dev_cntl(vdevice_t* dev, int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
    (void)dev;
    (void)from_pid;
    (void)p;

    if(cmd == FONT_DEV_LOAD) {
        return font_dev_load(in, ret);
    }
    else if(cmd == FONT_DEV_GET_GLYPH) {
        return font_dev_get_glyph(in, ret);
    }
    else if(cmd == FONT_DEV_GET_FACE) {
        return font_dev_get_face(in, ret);
    }
    else if(cmd == FONT_DEV_LIST) {
        return font_dev_list(in, ret);
    }
    else if(cmd == FONT_DEV_FREE) {
        return font_dev_free(in, ret);
    }
    return -1;
}

static int doargs(int argc, char* argv[]) {
    int c = 0;
    while (c != -1) {
        c = getopt (argc, argv, "lmncvho");
        if(c == -1)
            break;

        switch (c) {
        case 'l':
            _load_mode = FT_LOAD_TARGET_LIGHT;
            break;
        case 'n':
            _load_mode = FT_LOAD_TARGET_NORMAL;
            break;
        case 'h':
            _hinting = FT_LOAD_FORCE_AUTOHINT;
            break;
        case 'o':
            _hinting = FT_LOAD_NO_HINTING | FT_LOAD_NO_AUTOHINT;
            break;
        default:
            c = -1;
            break;
        }
    }
    return optind;
}

int main(int argc, char** argv) {
    _load_mode = FT_LOAD_TARGET_NORMAL;
    _hinting = FT_LOAD_NO_HINTING | FT_LOAD_NO_AUTOHINT;
    doargs(argc, argv);

    const char* mnt_point = "/dev/font";
    font_dev_init();

    vdevice_t dev;
    memset(&dev, 0, sizeof(vdevice_t));
    strcpy(dev.desc, "font");
    dev.dev_cntl = font_dev_cntl;
    dev.cmd = font_cmd;

    font_open(DEFAULT_SYSTEM_FONT, DEFAULT_SYSTEM_FONT_FILE);

    device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444, false);
    font_dev_quit();
    return 0;
}
