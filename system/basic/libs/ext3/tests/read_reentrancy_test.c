/* Device callbacks may yield to another sdfsd reader while filling metadata.
 * Nest a second public read at that exact point to reproduce the cache race
 * deterministically, without relying on host thread scheduling. */
#include <assert.h>
#include <stdio.h>
#ifndef EXT3_SOURCE
#define EXT3_SOURCE "../src/ext3fs.c"
#endif
#include EXT3_SOURCE

static ext3_t fs;
static EXT3_INODE other;
static int nest_read, indirect_reads, writes;

void klog(const char *format, ...) { (void)format; }
str_t* str_new(const char *s) { (void)s; abort(); }
void str_free(str_t *s) { (void)s; abort(); }
int32_t sd_set_block_size(uint32_t n) { (void)n; return 0; }
int32_t sd_set_max_sector_index(uint32_t n) { (void)n; return 0; }
void sd_set_buffer_size(uint32_t n) { (void)n; }
const char* vfs_dir_name(const char* s, char* d, uint32_t n) {
    (void)s; (void)d; (void)n; abort();
}
const char* vfs_file_name(const char* s, char* d, uint32_t n) {
    (void)s; (void)d; (void)n; abort();
}

static int32_t read_disk(int32_t block, void *buf) {
    uint32_t bytes = ext3_block_size(&fs);
    memset(buf, 0, bytes);
    if (block == 1 || block == 2) {
        indirect_reads++;
        for (unsigned i = 0; i < bytes / 4; i++)
            ((uint32_t *)buf)[i] = (block == 1 ? 100 : 200) + i;
        if (block == 1 && nest_read) {
            char nested[EXT3_MAX_BLOCK_SIZE];
            nest_read = 0;
            assert(ext3_read(&fs, &other, nested, bytes, 12 * bytes) == (int)bytes);
            for (unsigned i = 0; i < bytes; i++) assert(nested[i] == 'B');
        }
    } else if (block >= 100 && block < 200) {
        memset(buf, 'A', bytes);
    } else if (block >= 200 && block < 300) {
        memset(buf, 'B', bytes);
    } else if (block == 300) {
        memset(buf, 'C', bytes);
    } else {
        assert(!"unexpected disk block");
    }
    return 0;
}

static int32_t write_disk(int32_t block, const void *buf) {
    (void)block; (void)buf; writes++; return 0;
}

static void check_bytes(const char *buf, unsigned n, char expected) {
    for (unsigned i = 0; i < n; i++) assert(buf[i] == expected);
}

int main(void) {
    for (unsigned log = 0; log <= 2; log++) {
        memset(&fs, 0, sizeof(fs));
        fs.super.s_log_block_size = log;
        fs.read_block = read_disk;
        fs.write_block = write_disk;
        unsigned bytes = ext3_block_size(&fs);
        EXT3_INODE inode = {0};
        inode.i_block[12] = 1;
        inode.i_size = 16 * bytes;
        other = inode;
        other.i_block[12] = 2;
        char data[4 * EXT3_MAX_BLOCK_SIZE];

        /* Includes both the aligned fast path and a partial-block read. */
        for (int partial = 0; partial <= 1; partial++) {
            _cached_indirect_blk = _cached_indirect_dirty = 0;
            nest_read = 1;
            unsigned n = partial ? bytes - 17 : bytes;
            assert(ext3_read(&fs, &inode, data, n, 12 * bytes + partial) == (int)n);
            check_bytes(data, n, 'A');
            assert(!nest_read);
        }

        /* A sequential request should fetch its indirect block only once. */
        _cached_indirect_blk = _cached_indirect_dirty = 0;
        indirect_reads = 0;
        assert(ext3_read(&fs, &inode, data, 4 * bytes, 12 * bytes) == (int)(4 * bytes));
        check_bytes(data, 4 * bytes, 'A');
        assert(indirect_reads == 1);

        /* Readers must see deferred writes without evicting/flushing them. */
        _cached_indirect_blk = 1;
        _cached_indirect_dirty = 1;
        _cached_indirect_block[0] = 300;
        writes = 0;
        assert(ext3_read(&fs, &other, data, bytes, 12 * bytes) == (int)bytes);
        check_bytes(data, bytes, 'B');
        assert(_cached_indirect_blk == 1 && _cached_indirect_dirty == 1);
        assert(_cached_indirect_block[0] == 300 && writes == 0);
        assert(ext3_read(&fs, &inode, data, bytes, 12 * bytes) == (int)bytes);
        check_bytes(data, bytes, 'C');
        _cached_indirect_blk = _cached_indirect_dirty = 0;
        printf("ext3 read reentrancy: %u-byte blocks PASS\n", bytes);
    }
    return 0;
}
