#include <fat32/fat32fs.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/*----------------------------------------------------------------
 * FAT date/time <-> unix time
 *--------------------------------------------------------------*/
static int64_t days_from_civil(int32_t y, uint32_t m, uint32_t d) {
    y -= m <= 2;
    int32_t era = (y >= 0 ? y : y - 399) / 400;
    uint32_t yoe = (uint32_t)(y - era * 400);
    uint32_t doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    uint32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (int64_t)era * 146097 + (int64_t)doe - 719468;
}

static void civil_from_days(int64_t z, int32_t* year, uint32_t* month, uint32_t* day) {
    z += 719468;
    int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    uint32_t doe = (uint32_t)(z - era * 146097);
    uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    int32_t y = (int32_t)(yoe + era * 400);
    uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    uint32_t mp = (5 * doy + 2) / 153;
    uint32_t d = doy - (153 * mp + 2) / 5 + 1;
    uint32_t m = mp + (mp < 10 ? 3 : -9);
    *year = y + (m <= 2);
    *month = m;
    *day = d;
}

uint32_t fat32_dt2unix(uint16_t fdate, uint16_t ftime) {
    int32_t year = 1980 + ((fdate >> 9) & 0x7F);
    uint32_t month = (fdate >> 5) & 0x0F;
    uint32_t day = fdate & 0x1F;
    uint32_t hour = (ftime >> 11) & 0x1F;
    uint32_t min = (ftime >> 5) & 0x3F;
    uint32_t sec = (ftime & 0x1F) * 2;
    if(month == 0 || day == 0)
        return 0;
    int64_t days = days_from_civil(year, month, day);
    int64_t t = days * 86400 + hour * 3600 + min * 60 + sec;
    return (t <= 0) ? 0U : (uint32_t)t;
}

void fat32_unix2dt(uint32_t utime, uint16_t* fdate, uint16_t* ftime) {
    int32_t year;
    uint32_t month, day;
    civil_from_days((int64_t)(utime / 86400), &year, &month, &day);
    uint32_t rem = utime % 86400;
    uint32_t hour = rem / 3600;
    uint32_t min = (rem % 3600) / 60;
    uint32_t sec = rem % 60;
    if(year < 1980) {
        year = 1980; month = 1; day = 1;
        hour = min = sec = 0;
    }
    if(year > 2107)
        year = 2107;
    *fdate = (uint16_t)(((year - 1980) << 9) | (month << 5) | day);
    *ftime = (uint16_t)((hour << 11) | (min << 5) | (sec / 2));
}

static void fat32_now_dt(uint16_t* fdate, uint16_t* ftime) {
    time_t now = time(NULL);
    fat32_unix2dt((now <= 0) ? 0U : (uint32_t)now, fdate, ftime);
}

/*----------------------------------------------------------------
 * sector/cluster io
 *--------------------------------------------------------------*/
static int32_t fat32_read_sectors_io(fat32_t* fat, uint32_t sector, void* buf, uint32_t count) {
    char* p = (char*)buf;
    if(count == 0)
        return 0;
    if(fat->read_sectors != NULL)
        return fat->read_sectors((int32_t)sector, buf, count);
    for(uint32_t i = 0; i < count; i++) {
        if(fat->read_sector((int32_t)(sector + i), p + (i * FAT32_SECTOR_SIZE)) != 0)
            return -1;
    }
    return 0;
}

static int32_t fat32_write_sectors_io(fat32_t* fat, uint32_t sector, const void* buf, uint32_t count) {
    const char* p = (const char*)buf;
    for(uint32_t i = 0; i < count; i++) {
        if(fat->write_sector((int32_t)(sector + i), p + (i * FAT32_SECTOR_SIZE)) != 0)
            return -1;
    }
    return 0;
}

static inline int32_t cluster_valid(fat32_t* fat, uint32_t cluster) {
    return cluster >= 2 && cluster < (fat->total_clusters + 2);
}

static int32_t read_cluster(fat32_t* fat, uint32_t cluster, void* buf) {
    if(!cluster_valid(fat, cluster))
        return -1;
    return fat32_read_sectors_io(fat, fat32_cluster_to_sector(fat, cluster), buf, fat->bpb.sectors_per_cluster);
}

static int32_t write_cluster(fat32_t* fat, uint32_t cluster, const void* buf) {
    if(!cluster_valid(fat, cluster))
        return -1;
    return fat32_write_sectors_io(fat, fat32_cluster_to_sector(fat, cluster), buf, fat->bpb.sectors_per_cluster);
}

/*----------------------------------------------------------------
 * FAT access (one-sector write-back cache)
 *--------------------------------------------------------------*/
static int32_t fat_cache_sync(fat32_t* fat) {
    if(fat->fat_cache_dirty == 0 || fat->fat_cache_sector < 0)
        return 0;
    /* keep every FAT copy in sync */
    for(uint32_t i = 0; i < fat->bpb.num_fats; i++) {
        uint32_t sector = fat->fat_start_sector + i * fat->bpb.fat_size32 + (uint32_t)fat->fat_cache_sector;
        if(fat->write_sector((int32_t)sector, fat->fat_cache) != 0)
            return -1;
    }
    fat->fat_cache_dirty = 0;
    return 0;
}

static int32_t fat_cache_load(fat32_t* fat, uint32_t rel_sector) {
    if(fat->fat_cache_sector == (int32_t)rel_sector)
        return 0;
    if(fat_cache_sync(fat) != 0)
        return -1;
    if(fat->read_sector((int32_t)(fat->fat_start_sector + rel_sector), fat->fat_cache) != 0) {
        fat->fat_cache_sector = -1;
        return -1;
    }
    fat->fat_cache_sector = (int32_t)rel_sector;
    return 0;
}

static int32_t fat_get(fat32_t* fat, uint32_t cluster, uint32_t* val) {
    if(!cluster_valid(fat, cluster))
        return -1;
    uint32_t offset = cluster * 4;
    if(fat_cache_load(fat, offset / FAT32_SECTOR_SIZE) != 0)
        return -1;
    uint32_t v;
    memcpy(&v, fat->fat_cache + (offset % FAT32_SECTOR_SIZE), 4);
    *val = v & FAT32_ENTRY_MASK;
    return 0;
}

static int32_t fat_set(fat32_t* fat, uint32_t cluster, uint32_t val) {
    if(!cluster_valid(fat, cluster))
        return -1;
    uint32_t offset = cluster * 4;
    if(fat_cache_load(fat, offset / FAT32_SECTOR_SIZE) != 0)
        return -1;
    uint32_t v;
    memcpy(&v, fat->fat_cache + (offset % FAT32_SECTOR_SIZE), 4);
    v = (v & ~FAT32_ENTRY_MASK) | (val & FAT32_ENTRY_MASK);
    memcpy(fat->fat_cache + (offset % FAT32_SECTOR_SIZE), &v, 4);
    fat->fat_cache_dirty = 1;
    return 0;
}

/* the seq-th cluster of a chain, 0-based */
static int32_t chain_get(fat32_t* fat, uint32_t first, uint32_t seq, uint32_t* out) {
    uint32_t c = first;
    while(seq > 0) {
        uint32_t next;
        if(fat_get(fat, c, &next) != 0)
            return -1;
        if(fat32_is_eoc(next) || next == FAT32_ENTRY_FREE || next == FAT32_ENTRY_BAD)
            return -1;
        c = next;
        seq--;
    }
    if(!cluster_valid(fat, c))
        return -1;
    *out = c;
    return 0;
}

static int32_t cluster_zero(fat32_t* fat, uint32_t cluster) {
    uint8_t sector[FAT32_SECTOR_SIZE];
    memset(sector, 0, sizeof(sector));
    uint32_t start = fat32_cluster_to_sector(fat, cluster);
    for(uint32_t i = 0; i < fat->bpb.sectors_per_cluster; i++) {
        if(fat->write_sector((int32_t)(start + i), sector) != 0)
            return -1;
    }
    return 0;
}

static int32_t cluster_alloc(fat32_t* fat, int32_t zero, uint32_t* out) {
    uint32_t total = fat->total_clusters;
    uint32_t c = fat->free_hint;
    if(c < 2 || c >= total + 2)
        c = 2;

    for(uint32_t scanned = 0; scanned < total; scanned++) {
        uint32_t v;
        if(fat_get(fat, c, &v) != 0)
            return -1;
        if(v == FAT32_ENTRY_FREE) {
            if(fat_set(fat, c, FAT32_ENTRY_EOC) != 0)
                return -1;
            if(zero && cluster_zero(fat, c) != 0) {
                fat_set(fat, c, FAT32_ENTRY_FREE);
                return -1;
            }
            fat->free_hint = c + 1;
            *out = c;
            return 0;
        }
        c++;
        if(c >= total + 2)
            c = 2;
    }
    return -1; /* volume full */
}

static int32_t chain_free(fat32_t* fat, uint32_t first) {
    uint32_t c = first;
    while(cluster_valid(fat, c)) {
        uint32_t next;
        if(fat_get(fat, c, &next) != 0)
            return -1;
        if(fat_set(fat, c, FAT32_ENTRY_FREE) != 0)
            return -1;
        if(fat->free_hint > c)
            fat->free_hint = c;
        if(fat32_is_eoc(next) || next == FAT32_ENTRY_FREE || next == FAT32_ENTRY_BAD)
            break;
        c = next;
    }
    return 0;
}

/* append one cluster to the chain whose last cluster is 'last' */
static int32_t chain_extend(fat32_t* fat, uint32_t last, int32_t zero, uint32_t* out) {
    uint32_t c;
    if(cluster_alloc(fat, zero, &c) != 0)
        return -1;
    if(cluster_valid(fat, last)) {
        if(fat_set(fat, last, c) != 0) {
            fat_set(fat, c, FAT32_ENTRY_FREE);
            return -1;
        }
    }
    *out = c;
    return 0;
}

/*----------------------------------------------------------------
 * names: 8.3, LFN, utf8
 *--------------------------------------------------------------*/
static uint8_t sfn_checksum(const uint8_t* sfn) {
    uint8_t sum = 0;
    for(int32_t i = 0; i < 11; i++)
        sum = (uint8_t)(((sum & 1) ? 0x80 : 0) + (sum >> 1) + sfn[i]);
    return sum;
}

static char to_lower_c(char c) {
    return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
}

static char to_upper_c(char c) {
    return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
}

/* FAT names are case-insensitive (ASCII fold) */
static int32_t name_ieq(const char* a, const char* b) {
    while(*a != 0 && *b != 0) {
        if(to_lower_c(*a) != to_lower_c(*b))
            return 0;
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

static void sfn_to_name(const fat32_dirent_t* e, char* out) {
    int32_t n = 0;
    int32_t low_base = (e->nt_res & 0x08) != 0;
    int32_t low_ext = (e->nt_res & 0x10) != 0;

    for(int32_t i = 0; i < 8 && e->name[i] != ' '; i++) {
        char c = (char)e->name[i];
        if(n == 0 && (uint8_t)c == 0x05) /* 0x05 stores a real 0xE5 leading byte */
            c = (char)0xE5;
        out[n++] = low_base ? to_lower_c(c) : c;
    }
    if(e->name[8] != ' ') {
        out[n++] = '.';
        for(int32_t i = 8; i < 11 && e->name[i] != ' '; i++) {
            char c = (char)e->name[i];
            out[n++] = low_ext ? to_lower_c(c) : c;
        }
    }
    out[n] = 0;
}

/* ucs2 -> utf8, returns length written (excluding terminator) */
static int32_t ucs2_to_utf8(const uint16_t* ucs, uint32_t num, char* out, uint32_t maxlen) {
    uint32_t n = 0;
    for(uint32_t i = 0; i < num; i++) {
        uint16_t c = ucs[i];
        if(c == 0)
            break;
        if(c < 0x80) {
            if(n + 1 >= maxlen) break;
            out[n++] = (char)c;
        }
        else if(c < 0x800) {
            if(n + 2 >= maxlen) break;
            out[n++] = (char)(0xC0 | (c >> 6));
            out[n++] = (char)(0x80 | (c & 0x3F));
        }
        else {
            if(n + 3 >= maxlen) break;
            out[n++] = (char)(0xE0 | (c >> 12));
            out[n++] = (char)(0x80 | ((c >> 6) & 0x3F));
            out[n++] = (char)(0x80 | (c & 0x3F));
        }
    }
    out[n] = 0;
    return (int32_t)n;
}

/* utf8 -> ucs2, returns number of ucs2 chars, -1 on overflow */
static int32_t utf8_to_ucs2(const char* s, uint16_t* out, uint32_t max) {
    uint32_t n = 0;
    const uint8_t* p = (const uint8_t*)s;
    while(*p != 0) {
        uint32_t c;
        if(*p < 0x80) {
            c = *p;
            p += 1;
        }
        else if((*p & 0xE0) == 0xC0 && (p[1] & 0xC0) == 0x80) {
            c = ((uint32_t)(*p & 0x1F) << 6) | (p[1] & 0x3F);
            p += 2;
        }
        else if((*p & 0xF0) == 0xE0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) {
            c = ((uint32_t)(*p & 0x0F) << 12) | ((uint32_t)(p[1] & 0x3F) << 6) | (p[2] & 0x3F);
            p += 3;
        }
        else { /* out of ucs2 range or invalid, replace */
            c = '_';
            p += 1;
        }
        if(n >= max)
            return -1;
        out[n++] = (uint16_t)c;
    }
    return (int32_t)n;
}

static int32_t sfn_char_ok(char c) {
    if(c >= 'A' && c <= 'Z')
        return 1;
    if(c >= '0' && c <= '9')
        return 1;
    return strchr("$%'-_@~`!(){}^#&", c) != NULL;
}

/* try to fit 'name' into a plain 8.3 entry.
 * returns 1 on success and fills sfn/nt_res, 0 if a LFN is needed. */
static int32_t name_to_sfn_exact(const char* name, uint8_t* sfn, uint8_t* nt_res) {
    const char* dot = strrchr(name, '.');
    const char* base = name;
    uint32_t base_len, ext_len;

    if(dot == name) /* leading dot(hidden style) always needs LFN */
        dot = NULL;
    base_len = (dot == NULL) ? strlen(name) : (uint32_t)(dot - name);
    ext_len = (dot == NULL) ? 0 : strlen(dot + 1);

    if(base_len == 0 || base_len > 8 || ext_len > 3)
        return 0;
    if(dot != NULL && ext_len == 0) /* trailing dot */
        return 0;

    int32_t base_low = 0, base_up = 0, ext_low = 0, ext_up = 0;
    memset(sfn, ' ', 11);
    for(uint32_t i = 0; i < base_len; i++) {
        char c = base[i];
        if(c >= 'a' && c <= 'z') base_low = 1;
        if(c >= 'A' && c <= 'Z') base_up = 1;
        c = to_upper_c(c);
        if(!sfn_char_ok(c))
            return 0;
        sfn[i] = (uint8_t)c;
    }
    for(uint32_t i = 0; i < ext_len; i++) {
        char c = dot[1 + i];
        if(c >= 'a' && c <= 'z') ext_low = 1;
        if(c >= 'A' && c <= 'Z') ext_up = 1;
        c = to_upper_c(c);
        if(!sfn_char_ok(c))
            return 0;
        sfn[8 + i] = (uint8_t)c;
    }
    /* mixed case in one part cannot be recorded by nt_res */
    if((base_low && base_up) || (ext_low && ext_up))
        return 0;
    *nt_res = (uint8_t)((base_low ? 0x08 : 0) | (ext_low ? 0x10 : 0));
    return 1;
}

/* build the uppercase basis-name for a LFN-backed short entry */
static void name_to_sfn_basis(const char* name, uint8_t* sfn) {
    const char* dot = strrchr(name, '.');
    uint32_t n = 0;

    memset(sfn, ' ', 11);
    if(dot == name)
        dot = NULL;
    for(const char* p = name; *p != 0 && n < 8; p++) {
        if(dot != NULL && p >= dot)
            break;
        char c = to_upper_c(*p);
        if(c == ' ' || c == '.')
            continue;
        if(!sfn_char_ok(c))
            c = '_';
        sfn[n++] = (uint8_t)c;
    }
    if(n == 0)
        sfn[n++] = '_';
    if(dot != NULL) {
        n = 8;
        for(const char* p = dot + 1; *p != 0 && n < 11; p++) {
            char c = to_upper_c(*p);
            if(c == ' ' || c == '.')
                continue;
            if(!sfn_char_ok(c))
                c = '_';
            sfn[n++] = (uint8_t)c;
        }
    }
}

/*----------------------------------------------------------------
 * directory iteration
 *--------------------------------------------------------------*/
static int32_t diriter_open(fat32_t* fat, uint32_t first_cluster, fat32_dir_t* it) {
    memset(it, 0, sizeof(fat32_dir_t));
    it->first_cluster = first_cluster;
    it->cur_cluster = first_cluster;
    it->cbuf = (uint8_t*)malloc(fat->bytes_per_cluster);
    if(it->cbuf == NULL)
        return -1;
    return 0;
}

static void diriter_close(fat32_dir_t* it) {
    if(it->cbuf != NULL) {
        free(it->cbuf);
        it->cbuf = NULL;
    }
}

/* step to the next raw entry.
 * returns 1 and sets ent/index on success, 0 at end of chain, -1 on error */
static int32_t diriter_next_raw(fat32_t* fat, fat32_dir_t* it, fat32_dirent_t** ent, uint32_t* index) {
    uint32_t epc = fat32_dirents_per_cluster(fat);

    if(it->entry_in_cluster >= epc) {
        uint32_t next;
        if(fat_get(fat, it->cur_cluster, &next) != 0)
            return -1;
        if(fat32_is_eoc(next) || next == FAT32_ENTRY_FREE || next == FAT32_ENTRY_BAD)
            return 0;
        it->cur_cluster = next;
        it->cluster_seq++;
        it->entry_in_cluster = 0;
        it->cbuf_valid = 0;
    }
    if(!cluster_valid(fat, it->cur_cluster))
        return 0;
    if(!it->cbuf_valid) {
        if(read_cluster(fat, it->cur_cluster, it->cbuf) != 0)
            return -1;
        it->cbuf_valid = 1;
    }
    *ent = (fat32_dirent_t*)(it->cbuf + it->entry_in_cluster * FAT32_DIRENT_SIZE);
    *index = it->cluster_seq * epc + it->entry_in_cluster;
    it->entry_in_cluster++;
    return 1;
}

/* read the next real node (skips free entries and volume labels,
 * assembles long names). returns 1:got, 0:end, -1:error */
static int32_t diriter_next_node(fat32_t* fat, fat32_dir_t* it, fat32_node_t* out) {
    uint16_t lfn_ucs2[FAT32_LFN_CHARS * 20];
    uint32_t lfn_num = 0;
    uint8_t lfn_csum = 0;
    int32_t lfn_ok = 0;

    while(1) {
        fat32_dirent_t* e;
        uint32_t index;
        int32_t r = diriter_next_raw(fat, it, &e, &index);
        if(r <= 0)
            return r;

        if(e->name[0] == FAT32_DIRENT_END)
            return 0; /* everything after is free */
        if(e->name[0] == FAT32_DIRENT_FREE) {
            lfn_ok = 0;
            continue;
        }

        if((e->attr & FAT32_ATTR_LONG_NAME_MASK) == FAT32_ATTR_LONG_NAME) {
            fat32_lfn_t* l = (fat32_lfn_t*)e;
            uint32_t order = l->order & ~FAT32_LFN_LAST_FLAG;
            if(order == 0 || order > 20) { /* corrupted, drop */
                lfn_ok = 0;
                continue;
            }
            if(l->order & FAT32_LFN_LAST_FLAG) {
                memset(lfn_ucs2, 0, sizeof(lfn_ucs2));
                lfn_num = order;
                lfn_csum = l->checksum;
                lfn_ok = 1;
            }
            else if(!lfn_ok || l->checksum != lfn_csum) {
                lfn_ok = 0;
                continue;
            }
            uint16_t* dst = &lfn_ucs2[(order - 1) * FAT32_LFN_CHARS];
            memcpy(&dst[0], l->name1, 5 * 2);
            memcpy(&dst[5], l->name2, 6 * 2);
            memcpy(&dst[11], l->name3, 2 * 2);
            continue;
        }

        if(e->attr & FAT32_ATTR_VOLUME_ID) {
            lfn_ok = 0;
            continue;
        }

        /* a short entry closes the run */
        memset(out, 0, sizeof(fat32_node_t));
        out->start_cluster = fat32_dirent_cluster(e);
        out->size = (e->attr & FAT32_ATTR_DIRECTORY) ? 0 : e->file_size;
        out->attr = e->attr;
        out->crt_date = e->crt_date;
        out->crt_time = e->crt_time;
        out->wrt_date = e->wrt_date;
        out->wrt_time = e->wrt_time;
        out->dirent_dir_cluster = it->first_cluster;
        out->dirent_index = index;

        if(lfn_ok && lfn_csum == sfn_checksum(e->name)) {
            out->dirent_lfn_num = lfn_num;
            ucs2_to_utf8(lfn_ucs2, lfn_num * FAT32_LFN_CHARS, out->name, FAT32_NAME_MAX);
        }
        else {
            out->dirent_lfn_num = 0;
            sfn_to_name(e, out->name);
        }
        if(out->name[0] == 0) /* refuse degenerated names */
            continue;
        return 1;
    }
}

/*----------------------------------------------------------------
 * directory entry random access (by linear index)
 *--------------------------------------------------------------*/
static int32_t dirent_locate(fat32_t* fat, uint32_t dir_cluster, uint32_t index,
        uint32_t* sector, uint32_t* offset) {
    uint32_t epc = fat32_dirents_per_cluster(fat);
    uint32_t cluster;
    if(chain_get(fat, dir_cluster, index / epc, &cluster) != 0)
        return -1;
    uint32_t in_cluster = (index % epc) * FAT32_DIRENT_SIZE;
    *sector = fat32_cluster_to_sector(fat, cluster) + in_cluster / FAT32_SECTOR_SIZE;
    *offset = in_cluster % FAT32_SECTOR_SIZE;
    return 0;
}

static int32_t dirent_read_at(fat32_t* fat, uint32_t dir_cluster, uint32_t index, fat32_dirent_t* e) {
    uint8_t sector_buf[FAT32_SECTOR_SIZE];
    uint32_t sector, offset;
    if(dirent_locate(fat, dir_cluster, index, &sector, &offset) != 0)
        return -1;
    if(fat->read_sector((int32_t)sector, sector_buf) != 0)
        return -1;
    memcpy(e, sector_buf + offset, FAT32_DIRENT_SIZE);
    return 0;
}

static int32_t dirent_write_at(fat32_t* fat, uint32_t dir_cluster, uint32_t index, const fat32_dirent_t* e) {
    uint8_t sector_buf[FAT32_SECTOR_SIZE];
    uint32_t sector, offset;
    if(dirent_locate(fat, dir_cluster, index, &sector, &offset) != 0)
        return -1;
    if(fat->read_sector((int32_t)sector, sector_buf) != 0)
        return -1;
    memcpy(sector_buf + offset, e, FAT32_DIRENT_SIZE);
    if(fat->write_sector((int32_t)sector, sector_buf) != 0)
        return -1;
    return 0;
}

/*----------------------------------------------------------------
 * lookup
 *--------------------------------------------------------------*/
static int32_t fat32_lookup(fat32_t* fat, uint32_t dir_cluster, const char* name, fat32_node_t* out) {
    fat32_dir_t it;
    fat32_node_t node;
    int32_t found = -1;

    if(diriter_open(fat, dir_cluster, &it) != 0)
        return -1;
    while(diriter_next_node(fat, &it, &node) == 1) {
        if(name_ieq(node.name, name)) {
            if(out != NULL)
                memcpy(out, &node, sizeof(fat32_node_t));
            found = 0;
            break;
        }
    }
    diriter_close(&it);
    return found;
}

void fat32_root_node(fat32_t* fat, fat32_node_t* node) {
    memset(node, 0, sizeof(fat32_node_t));
    node->start_cluster = fat->bpb.root_cluster;
    node->attr = FAT32_ATTR_DIRECTORY;
    node->name[0] = '/';
}

int32_t fat32_node_by_fname(fat32_t* fat, const char* fname, fat32_node_t* node) {
    char path[FAT32_NAME_MAX];
    fat32_node_t cur;

    fat32_root_node(fat, &cur);
    if(fname == NULL || fname[0] == 0 || strcmp(fname, "/") == 0) {
        memcpy(node, &cur, sizeof(fat32_node_t));
        return 0;
    }

    strncpy(path, fname, FAT32_NAME_MAX - 1);
    path[FAT32_NAME_MAX - 1] = 0;

    char* p = path;
    while(*p != 0) {
        while(*p == '/')
            p++;
        if(*p == 0)
            break;
        char* start = p;
        while(*p != 0 && *p != '/')
            p++;
        if(*p != 0)
            *p++ = 0;

        if((cur.attr & FAT32_ATTR_DIRECTORY) == 0)
            return -1;
        if(fat32_lookup(fat, cur.start_cluster, start, &cur) != 0)
            return -1;
    }
    memcpy(node, &cur, sizeof(fat32_node_t));
    return 0;
}

/*----------------------------------------------------------------
 * public dir iteration
 *--------------------------------------------------------------*/
int32_t fat32_diropen(fat32_t* fat, fat32_node_t* dir_node, fat32_dir_t* it) {
    if((dir_node->attr & FAT32_ATTR_DIRECTORY) == 0)
        return -1;
    return diriter_open(fat, dir_node->start_cluster, it);
}

int32_t fat32_dirnext(fat32_t* fat, fat32_dir_t* it, fat32_node_t* out) {
    while(1) {
        int32_t r = diriter_next_node(fat, it, out);
        if(r != 1)
            return r;
        if(strcmp(out->name, ".") == 0 || strcmp(out->name, "..") == 0)
            continue;
        return 1;
    }
}

void fat32_dirclose(fat32_dir_t* it) {
    diriter_close(it);
}

/*----------------------------------------------------------------
 * file read/write
 *--------------------------------------------------------------*/
int32_t fat32_read(fat32_t* fat, fat32_node_t* node, char* buf, int32_t nbytes, int32_t offset) {
    if(nbytes <= 0 || offset < 0)
        return 0;
    if((node->attr & FAT32_ATTR_DIRECTORY) == 0) {
        if((uint32_t)offset >= node->size)
            return 0;
        if((uint32_t)(offset + nbytes) > node->size)
            nbytes = (int32_t)(node->size - (uint32_t)offset);
    }
    if(node->start_cluster == 0)
        return 0;

    uint32_t bpc = fat->bytes_per_cluster;
    uint32_t cluster;
    if(chain_get(fat, node->start_cluster, (uint32_t)offset / bpc, &cluster) != 0)
        return -1;

    uint8_t* cbuf = (uint8_t*)malloc(bpc);
    if(cbuf == NULL)
        return -1;

    uint32_t in_off = (uint32_t)offset % bpc;
    int32_t done = 0;
    while(done < nbytes) {
        uint32_t chunk = bpc - in_off;
        if(chunk > (uint32_t)(nbytes - done))
            chunk = (uint32_t)(nbytes - done);

        if(in_off == 0 && chunk == bpc) {
            /* full aligned cluster straight into the caller buffer */
            if(fat32_read_sectors_io(fat, fat32_cluster_to_sector(fat, cluster),
                    buf + done, fat->bpb.sectors_per_cluster) != 0)
                break;
        }
        else {
            if(read_cluster(fat, cluster, cbuf) != 0)
                break;
            memcpy(buf + done, cbuf + in_off, chunk);
        }
        done += (int32_t)chunk;
        in_off = 0;

        if(done < nbytes) {
            uint32_t next;
            if(fat_get(fat, cluster, &next) != 0 || fat32_is_eoc(next) ||
                    next == FAT32_ENTRY_FREE || next == FAT32_ENTRY_BAD)
                break;
            cluster = next;
        }
    }
    free(cbuf);
    return done;
}

/* make sure the chain covers the seq-th cluster, extending with
 * zeroed clusters when needed. */
static int32_t chain_ensure(fat32_t* fat, fat32_node_t* node, uint32_t seq, uint32_t* out) {
    uint32_t c;

    if(node->start_cluster == 0) {
        if(cluster_alloc(fat, 1, &c) != 0)
            return -1;
        node->start_cluster = c;
    }
    c = node->start_cluster;
    for(uint32_t i = 0; i < seq; i++) {
        uint32_t next;
        if(fat_get(fat, c, &next) != 0)
            return -1;
        if(fat32_is_eoc(next) || next == FAT32_ENTRY_FREE || next == FAT32_ENTRY_BAD) {
            if(chain_extend(fat, c, 1, &next) != 0)
                return -1;
        }
        c = next;
    }
    *out = c;
    return 0;
}

int32_t fat32_write(fat32_t* fat, fat32_node_t* node, const char* data, int32_t nbytes, int32_t offset) {
    if(nbytes < 0 || offset < 0)
        return -1;
    if(nbytes == 0)
        return 0;

    uint32_t bpc = fat->bytes_per_cluster;
    uint8_t* cbuf = (uint8_t*)malloc(bpc);
    if(cbuf == NULL)
        return -1;

    uint32_t seq = (uint32_t)offset / bpc;
    uint32_t in_off = (uint32_t)offset % bpc;
    uint32_t cluster = 0;
    int32_t done = 0;

    if(chain_ensure(fat, node, seq, &cluster) != 0) {
        free(cbuf);
        return -1;
    }

    while(done < nbytes) {
        uint32_t chunk = bpc - in_off;
        if(chunk > (uint32_t)(nbytes - done))
            chunk = (uint32_t)(nbytes - done);

        if(in_off == 0 && chunk == bpc) {
            if(write_cluster(fat, cluster, data + done) != 0)
                break;
        }
        else {
            if(read_cluster(fat, cluster, cbuf) != 0)
                break;
            memcpy(cbuf + in_off, data + done, chunk);
            if(write_cluster(fat, cluster, cbuf) != 0)
                break;
        }
        done += (int32_t)chunk;
        in_off = 0;

        if(done < nbytes) {
            uint32_t next;
            if(fat_get(fat, cluster, &next) != 0)
                break;
            if(fat32_is_eoc(next) || next == FAT32_ENTRY_FREE || next == FAT32_ENTRY_BAD) {
                if(chain_extend(fat, cluster, 0, &next) != 0)
                    break;
            }
            cluster = next;
        }
    }
    free(cbuf);

    if(done > 0) {
        if((node->attr & FAT32_ATTR_DIRECTORY) == 0 &&
                (uint32_t)(offset + done) > node->size)
            node->size = (uint32_t)(offset + done);
        fat32_now_dt(&node->wrt_date, &node->wrt_time);
    }
    if(fat_cache_sync(fat) != 0)
        return -1;
    return (done == 0) ? -1 : done;
}

/*----------------------------------------------------------------
 * metadata write-back
 *--------------------------------------------------------------*/
int32_t fat32_update_node(fat32_t* fat, fat32_node_t* node) {
    if(node->dirent_dir_cluster == 0) /* root has no dirent */
        return 0;

    fat32_dirent_t e;
    if(dirent_read_at(fat, node->dirent_dir_cluster, node->dirent_index, &e) != 0)
        return -1;
    e.attr = node->attr;
    e.fst_clus_lo = (uint16_t)(node->start_cluster & 0xFFFF);
    e.fst_clus_hi = (uint16_t)(node->start_cluster >> 16);
    e.file_size = (node->attr & FAT32_ATTR_DIRECTORY) ? 0 : node->size;
    e.wrt_date = node->wrt_date;
    e.wrt_time = node->wrt_time;
    if(dirent_write_at(fat, node->dirent_dir_cluster, node->dirent_index, &e) != 0)
        return -1;
    return fat_cache_sync(fat);
}

int32_t fat32_truncate(fat32_t* fat, fat32_node_t* node) {
    if(node->attr & FAT32_ATTR_DIRECTORY)
        return -1;
    if(node->start_cluster != 0) {
        if(chain_free(fat, node->start_cluster) != 0)
            return -1;
        node->start_cluster = 0;
    }
    node->size = 0;
    fat32_now_dt(&node->wrt_date, &node->wrt_time);
    if(fat32_update_node(fat, node) != 0)
        return -1;
    return fat_cache_sync(fat);
}

/*----------------------------------------------------------------
 * create
 *--------------------------------------------------------------*/
static int32_t dir_sfn_exists(fat32_t* fat, uint32_t dir_cluster, const uint8_t* sfn) {
    fat32_dir_t it;
    fat32_dirent_t* e;
    uint32_t index;
    int32_t found = 0;

    if(diriter_open(fat, dir_cluster, &it) != 0)
        return -1;
    while(diriter_next_raw(fat, &it, &e, &index) == 1) {
        if(e->name[0] == FAT32_DIRENT_END)
            break;
        if(e->name[0] == FAT32_DIRENT_FREE)
            continue;
        if((e->attr & FAT32_ATTR_LONG_NAME_MASK) == FAT32_ATTR_LONG_NAME)
            continue;
        if(memcmp(e->name, sfn, 11) == 0) {
            found = 1;
            break;
        }
    }
    diriter_close(&it);
    return found;
}

/* generate a unique "BASIS~N" short name */
static int32_t gen_unique_sfn(fat32_t* fat, uint32_t dir_cluster, const char* name, uint8_t* sfn) {
    name_to_sfn_basis(name, sfn);
    for(uint32_t n = 1; n < 1000000; n++) {
        char tail[12];
        snprintf(tail, sizeof(tail), "~%u", n);
        uint32_t tail_len = strlen(tail);
        uint32_t base_len = 8 - tail_len;
        uint8_t cand[11];
        memcpy(cand, sfn, 11);
        /* find real basis length */
        uint32_t bl = 0;
        while(bl < 8 && cand[bl] != ' ')
            bl++;
        if(bl > base_len)
            bl = base_len;
        memcpy(cand + bl, tail, tail_len);
        for(uint32_t i = bl + tail_len; i < 8; i++)
            cand[i] = ' ';
        int32_t r = dir_sfn_exists(fat, dir_cluster, cand);
        if(r < 0)
            return -1;
        if(r == 0) {
            memcpy(sfn, cand, 11);
            return 0;
        }
    }
    return -1;
}

/* find 'need' consecutive free entries, extending the directory
 * with zeroed clusters when the chain runs out. */
static int32_t dir_find_free_run(fat32_t* fat, uint32_t dir_cluster, uint32_t need, uint32_t* out_index) {
    fat32_dir_t it;
    fat32_dirent_t* e;
    uint32_t index = 0;
    uint32_t run_start = 0;
    uint32_t run = 0;
    uint32_t last_index = 0;
    int32_t r;

    if(diriter_open(fat, dir_cluster, &it) != 0)
        return -1;
    while((r = diriter_next_raw(fat, &it, &e, &index)) == 1) {
        last_index = index;
        if(e->name[0] == FAT32_DIRENT_END || e->name[0] == FAT32_DIRENT_FREE) {
            if(run == 0)
                run_start = index;
            run++;
            if(run >= need) {
                diriter_close(&it);
                *out_index = run_start;
                return 0;
            }
        }
        else {
            run = 0;
        }
    }
    uint32_t last_cluster = it.cur_cluster;
    diriter_close(&it);
    if(r < 0)
        return -1;

    /* end of chain reached: extend with zeroed clusters */
    if(run == 0)
        run_start = last_index + 1;
    uint32_t epc = fat32_dirents_per_cluster(fat);
    while(run < need) {
        uint32_t nc;
        if(chain_extend(fat, last_cluster, 1, &nc) != 0)
            return -1;
        last_cluster = nc;
        run += epc;
    }
    if(fat_cache_sync(fat) != 0)
        return -1;
    *out_index = run_start;
    return 0;
}

static int32_t fat32_create(fat32_t* fat, fat32_node_t* dir_node, const char* name,
        int32_t is_dir, fat32_node_t* out) {
    if(dir_node == NULL || (dir_node->attr & FAT32_ATTR_DIRECTORY) == 0)
        return -1;
    if(name == NULL || name[0] == 0 || strlen(name) >= FAT32_NAME_MAX)
        return -1;
    if(fat32_lookup(fat, dir_node->start_cluster, name, NULL) == 0)
        return -1; /* already exists */

    uint8_t sfn[11];
    uint8_t nt_res = 0;
    uint32_t lfn_num = 0;
    uint16_t ucs2[FAT32_LFN_CHARS * 20 + 1];

    if(!name_to_sfn_exact(name, sfn, &nt_res)) {
        int32_t ulen = utf8_to_ucs2(name, ucs2, FAT32_LFN_CHARS * 20);
        if(ulen <= 0)
            return -1;
        lfn_num = ((uint32_t)ulen + FAT32_LFN_CHARS - 1) / FAT32_LFN_CHARS;
        /* pad the tail: terminator then 0xFFFF */
        for(uint32_t i = (uint32_t)ulen; i < lfn_num * FAT32_LFN_CHARS; i++)
            ucs2[i] = (i == (uint32_t)ulen) ? 0x0000 : 0xFFFF;
        nt_res = 0;
        if(gen_unique_sfn(fat, dir_node->start_cluster, name, sfn) != 0)
            return -1;
    }

    uint32_t first_cluster = 0;
    if(is_dir) {
        /* materialize the new directory on disk right away:
         * zeroed cluster with "." and ".." entries */
        if(cluster_alloc(fat, 1, &first_cluster) != 0)
            return -1;
    }

    uint32_t index;
    if(dir_find_free_run(fat, dir_node->start_cluster, lfn_num + 1, &index) != 0) {
        if(first_cluster != 0)
            chain_free(fat, first_cluster);
        fat_cache_sync(fat);
        return -1;
    }

    uint16_t fdate, ftime;
    fat32_now_dt(&fdate, &ftime);

    /* short entry */
    fat32_dirent_t se;
    memset(&se, 0, sizeof(se));
    memcpy(se.name, sfn, 11);
    se.attr = is_dir ? FAT32_ATTR_DIRECTORY : FAT32_ATTR_ARCHIVE;
    se.nt_res = nt_res;
    se.crt_date = fdate;
    se.crt_time = ftime;
    se.lst_acc_date = fdate;
    se.wrt_date = fdate;
    se.wrt_time = ftime;
    se.fst_clus_lo = (uint16_t)(first_cluster & 0xFFFF);
    se.fst_clus_hi = (uint16_t)(first_cluster >> 16);
    se.file_size = 0;

    /* LFN entries, highest order first */
    uint8_t csum = sfn_checksum(sfn);
    for(uint32_t i = 0; i < lfn_num; i++) {
        uint32_t order = lfn_num - i;
        fat32_lfn_t l;
        memset(&l, 0, sizeof(l));
        l.order = (uint8_t)(order | ((i == 0) ? FAT32_LFN_LAST_FLAG : 0));
        l.attr = FAT32_ATTR_LONG_NAME;
        l.checksum = csum;
        const uint16_t* src = &ucs2[(order - 1) * FAT32_LFN_CHARS];
        memcpy(l.name1, &src[0], 5 * 2);
        memcpy(l.name2, &src[5], 6 * 2);
        memcpy(l.name3, &src[11], 2 * 2);
        if(dirent_write_at(fat, dir_node->start_cluster, index + i, (fat32_dirent_t*)&l) != 0)
            return -1;
    }
    if(dirent_write_at(fat, dir_node->start_cluster, index + lfn_num, &se) != 0)
        return -1;

    if(is_dir) {
        /* "." and ".." must be on disk before the tree is ever rebuilt */
        uint8_t* cbuf = (uint8_t*)malloc(fat->bytes_per_cluster);
        if(cbuf == NULL)
            return -1;
        memset(cbuf, 0, fat->bytes_per_cluster);
        fat32_dirent_t* dot = (fat32_dirent_t*)cbuf;
        memcpy(dot, &se, sizeof(se));
        memset(dot->name, ' ', 11);
        dot->name[0] = '.';
        dot->nt_res = 0;

        fat32_dirent_t* dotdot = dot + 1;
        memcpy(dotdot, &se, sizeof(se));
        memset(dotdot->name, ' ', 11);
        dotdot->name[0] = '.';
        dotdot->name[1] = '.';
        dotdot->nt_res = 0;
        /* per spec, ".." pointing at root records cluster 0 */
        uint32_t parent = (dir_node->start_cluster == fat->bpb.root_cluster) ? 0 : dir_node->start_cluster;
        dotdot->fst_clus_lo = (uint16_t)(parent & 0xFFFF);
        dotdot->fst_clus_hi = (uint16_t)(parent >> 16);

        int32_t wr = write_cluster(fat, first_cluster, cbuf);
        free(cbuf);
        if(wr != 0)
            return -1;
    }

    if(fat_cache_sync(fat) != 0)
        return -1;

    if(out != NULL) {
        memset(out, 0, sizeof(fat32_node_t));
        out->start_cluster = first_cluster;
        out->size = 0;
        out->attr = se.attr;
        out->crt_date = fdate;
        out->crt_time = ftime;
        out->wrt_date = fdate;
        out->wrt_time = ftime;
        out->dirent_dir_cluster = dir_node->start_cluster;
        out->dirent_index = index + lfn_num;
        out->dirent_lfn_num = lfn_num;
        strncpy(out->name, name, FAT32_NAME_MAX - 1);
    }
    return 0;
}

int32_t fat32_create_dir(fat32_t* fat, fat32_node_t* dir_node, const char* name, fat32_node_t* out) {
    return fat32_create(fat, dir_node, name, 1, out);
}

int32_t fat32_create_file(fat32_t* fat, fat32_node_t* dir_node, const char* name, fat32_node_t* out) {
    return fat32_create(fat, dir_node, name, 0, out);
}

/*----------------------------------------------------------------
 * unlink/rmdir
 *--------------------------------------------------------------*/
static int32_t dir_delete_entries(fat32_t* fat, fat32_node_t* node) {
    uint32_t first = node->dirent_index - node->dirent_lfn_num;
    for(uint32_t i = 0; i <= node->dirent_lfn_num; i++) {
        fat32_dirent_t e;
        if(dirent_read_at(fat, node->dirent_dir_cluster, first + i, &e) != 0)
            return -1;
        e.name[0] = FAT32_DIRENT_FREE;
        if(dirent_write_at(fat, node->dirent_dir_cluster, first + i, &e) != 0)
            return -1;
    }
    return 0;
}

static int32_t dir_is_empty(fat32_t* fat, fat32_node_t* dir_node) {
    fat32_dir_t it;
    fat32_node_t kid;
    int32_t r;

    if(fat32_diropen(fat, dir_node, &it) != 0)
        return -1;
    r = fat32_dirnext(fat, &it, &kid);
    fat32_dirclose(&it);
    if(r < 0)
        return -1;
    return (r == 0) ? 1 : 0;
}

int32_t fat32_unlink(fat32_t* fat, const char* fname) {
    fat32_node_t node;
    if(fat32_node_by_fname(fat, fname, &node) != 0)
        return -1;
    if(node.attr & FAT32_ATTR_DIRECTORY)
        return -1;
    if(node.dirent_dir_cluster == 0)
        return -1;
    if(dir_delete_entries(fat, &node) != 0)
        return -1;
    if(node.start_cluster != 0)
        chain_free(fat, node.start_cluster);
    return fat_cache_sync(fat);
}

int32_t fat32_rmdir(fat32_t* fat, const char* fname) {
    fat32_node_t node;
    if(fat32_node_by_fname(fat, fname, &node) != 0)
        return -1;
    if((node.attr & FAT32_ATTR_DIRECTORY) == 0)
        return -1;
    if(node.dirent_dir_cluster == 0) /* root */
        return -1;
    if(dir_is_empty(fat, &node) != 1)
        return -1;
    if(dir_delete_entries(fat, &node) != 0)
        return -1;
    if(node.start_cluster != 0)
        chain_free(fat, node.start_cluster);
    return fat_cache_sync(fat);
}

/*----------------------------------------------------------------
 * whole-file read helper
 *--------------------------------------------------------------*/
void* fat32_readfile(fat32_t* fat, const char* fname, int32_t* size) {
    fat32_node_t node;
    if(size != NULL)
        *size = -1;
    if(fat32_node_by_fname(fat, fname, &node) != 0)
        return NULL;
    if(node.attr & FAT32_ATTR_DIRECTORY)
        return NULL;

    char* buf = (char*)malloc(node.size == 0 ? 1 : node.size);
    if(buf == NULL)
        return NULL;
    if(node.size > 0) {
        int32_t rd = fat32_read(fat, &node, buf, (int32_t)node.size, 0);
        if(rd != (int32_t)node.size) {
            free(buf);
            return NULL;
        }
    }
    if(size != NULL)
        *size = (int32_t)node.size;
    return buf;
}

/*----------------------------------------------------------------
 * init/quit
 *--------------------------------------------------------------*/
int32_t fat32_flush(fat32_t* fat) {
    return fat_cache_sync(fat);
}

static int32_t fat32_validate(fat32_t* fat, const uint8_t* sector0) {
    const fat32_bpb_t* bpb = &fat->bpb;
    if(sector0[510] != 0x55 || sector0[511] != 0xAA)
        return -1;
    if(bpb->bytes_per_sector != FAT32_SECTOR_SIZE)
        return -1;
    uint8_t spc = bpb->sectors_per_cluster;
    if(spc == 0 || (spc & (spc - 1)) != 0 || spc > 128)
        return -1;
    /* FAT32 signature: no 16bit fat/root, versioned 0 */
    if(bpb->fat_size32 == 0 || bpb->fat_size16 != 0)
        return -1;
    if(bpb->root_entry_count != 0 || bpb->fs_version != 0)
        return -1;
    if(bpb->num_fats == 0 || bpb->reserved_sector_count == 0)
        return -1;
    if(bpb->total_sectors32 == 0 && bpb->total_sectors16 == 0)
        return -1;
    if(bpb->root_cluster < 2)
        return -1;
    return 0;
}

int32_t fat32_init_ex(fat32_t* fat, fat32_read_sector_func_t read_sector,
        fat32_read_sectors_func_t read_sectors, fat32_write_sector_func_t write_sector) {
    uint8_t sector0[FAT32_SECTOR_SIZE];

    memset(fat, 0, sizeof(fat32_t));
    fat->read_sector = read_sector;
    fat->read_sectors = read_sectors;
    fat->write_sector = write_sector;
    fat->fat_cache_sector = -1;

    if(read_sector(0, sector0) != 0)
        return -1;
    memcpy(&fat->bpb, sector0, sizeof(fat32_bpb_t));
    if(fat32_validate(fat, sector0) != 0)
        return -1;

    uint32_t total_sectors = (fat->bpb.total_sectors32 != 0) ?
            fat->bpb.total_sectors32 : fat->bpb.total_sectors16;
    fat->fat_start_sector = fat->bpb.reserved_sector_count;
    fat->data_start_sector = fat->fat_start_sector + fat->bpb.num_fats * fat->bpb.fat_size32;
    if(fat->data_start_sector >= total_sectors)
        return -1;
    fat->bytes_per_cluster = (uint32_t)fat->bpb.sectors_per_cluster * FAT32_SECTOR_SIZE;
    fat->total_clusters = (total_sectors - fat->data_start_sector) / fat->bpb.sectors_per_cluster;
    if(fat->total_clusters < 65525) /* smaller volumes are FAT12/16 */
        return -1;
    fat->free_hint = 2;

    fat->fat_cache = (uint8_t*)malloc(FAT32_SECTOR_SIZE);
    if(fat->fat_cache == NULL)
        return -1;
    return 0;
}

int32_t fat32_init(fat32_t* fat, fat32_read_sector_func_t read_sector, fat32_write_sector_func_t write_sector) {
    return fat32_init_ex(fat, read_sector, NULL, write_sector);
}

void fat32_quit(fat32_t* fat) {
    if(fat->fat_cache != NULL) {
        fat_cache_sync(fat);
        free(fat->fat_cache);
        fat->fat_cache = NULL;
    }
}
