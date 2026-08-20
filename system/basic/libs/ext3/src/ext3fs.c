#include <sd/sd.h>
#include <ext3/ext3fs.h>
#include <ewoksys/vfs.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ewoksys/mstr.h>

/*
 * ext3 filesystem operations.
 *
 * Ported from the ext2 library: on-disk structures and every algorithm
 * are identical, the difference is where writes go.
 *
 *  - file DATA blocks go straight to their home location on the device
 *    (ordered data mode): the SD layer's writes are synchronous, so data
 *    is always durable before the metadata that references it is
 *    committed to the journal.
 *  - METADATA blocks (super, group descriptors, bitmaps, inode tables,
 *    directory blocks, indirect blocks) go through ext3_write_meta_blk:
 *    with a journal they are buffered in the dirty-block cache and
 *    journaled by ext3_commit(); on a plain ext2 filesystem the write
 *    goes directly to disk, giving exactly the old ext2 behaviour.
 *  - reads that may hit metadata use ext3_read_blk so they observe the
 *    cached (written-but-not-yet-committed) contents.
 *
 * Transaction protocol (one transaction at a time + immediate
 * checkpoint): ext3_commit() journals the cached blocks (revoke records
 * first, then descriptor+data blocks, then the commit block), publishes
 * the transaction in the journal superblock, writes the blocks back to
 * their home locations and resets the log.  A crash at any point is
 * repaired by replaying the last committed transaction at the next
 * mount (jbd_recover()).
 *
 * NOTE: the kernel reads files straight from the sd card before sdfsd
 * starts and does not replay the journal; after an unclean shutdown the
 * boot-time view may be stale until sdfsd mounts and recovers.
 */

#define SHORT_NAME_MAX 64

static int32_t ext3_bdealloc(ext3_t* ext3, uint32_t block);
static int32_t need_len(int32_t len);
static int32_t ext3_ensure_data_block(ext3_t* ext3, EXT3_INODE* node, int32_t lbk, int32_t* blk, int32_t* is_new);
static int32_t ext3_flush_meta(ext3_t* ext3);
static int32_t ext3_checkpoint_cache(ext3_t* ext3);
static char* ext3_get_block_bitmap(ext3_t* ext3, uint32_t blk);
static char* ext3_get_inode_bitmap(ext3_t* ext3, uint32_t blk);
static uint32_t* ext3_get_cached_indirect_block(ext3_t* ext3, uint32_t blk);
static int32_t ext3_get_data_block(ext3_t* ext3, EXT3_INODE* node, int32_t lbk, int32_t* blk);

/* RAM caches for the "one bitmap / one indirect block at a time" access
 * pattern (inherited from the ext2 library): a multi-block allocation
 * touches the same bitmap block over and over, deferring its write-back
 * keeps that at one write per flush.  Their write-back is routed through
 * ext3_write_meta_blk so journal mode defers it to the commit. */
static uint32_t _cached_block_bitmap_blk = 0;
static uint8_t _cached_block_bitmap_dirty = 0;
static char* _cached_block_bitmap = NULL;
static uint32_t _cached_inode_bitmap_blk = 0;
static uint8_t _cached_inode_bitmap_dirty = 0;
static char* _cached_inode_bitmap = NULL;
static uint32_t _cached_indirect_blk = 0;
static uint8_t _cached_indirect_dirty = 0;
static uint32_t _cached_indirect_block[EXT3_MAX_BLOCK_SIZE / sizeof(uint32_t)];

/* guards ext3_commit() against re-entering itself through the
 * auto-commit of ext3_cache_set() while flushing deferred metadata */
static uint8_t _in_commit = 0;

static int32_t ext3_validate_super(ext3_t* ext3) {
    uint32_t unsupported_incompat = ext3->super.s_feature_incompat &
        ~(EXT2_FEATURE_INCOMPAT_FILETYPE | EXT3_FEATURE_INCOMPAT_RECOVER);
    uint32_t unsupported_ro = ext3->super.s_feature_ro_compat &
        ~(EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER | EXT2_FEATURE_RO_COMPAT_LARGE_FILE);
    uint32_t unsupported_compat = ext3->super.s_feature_compat &
        ~(EXT3_FEATURE_COMPAT_HAS_JOURNAL | EXT2_FEATURE_COMPAT_EXT_ATTR |
          EXT2_FEATURE_COMPAT_RESIZE_INODE | EXT2_FEATURE_COMPAT_DIR_INDEX);

    if(ext3->super.s_magic != EXT3_SUPER_MAGIC)
        return -1;
    uint32_t block_size = ext3_block_size(ext3);
    if(block_size != 1024 && block_size != 2048 && block_size != 4096)
        return -1;
    if(ext3_inode_size(ext3) < sizeof(EXT3_INODE))
        return -1;
    /* we read/write whole 32-byte group descriptors */
    if(ext3->super.s_desc_size != 0 && ext3->super.s_desc_size != sizeof(EXT3_GD))
        return -1;
    /* RECOVER is accepted: it just means the previous mount did not
     * finish cleanly and the journal must be replayed (ext2 images
     * carry none of the ext3 features, so they still validate) */
    if(unsupported_incompat != 0 || unsupported_ro != 0 || unsupported_compat != 0)
        return -1;
    return 0;
}

static uint32_t ext3_now(void) {
    time_t now = time(NULL);
    return (now <= 0) ? 0U : (uint32_t)now;
}

static int32_t ext3_dirent_name_equals(const EXT3_DIR_T* dp, const char* name) {
    size_t len = strlen(name);
    return dp->name_len == len && memcmp(dp->name, name, len) == 0;
}

static uint32_t ext3_dir_block_count(ext3_t* ext3, const EXT3_INODE* node) {
    uint32_t block_size = ext3_block_size(ext3);
    if(node->i_size == 0)
        return 0;
    return (node->i_size + block_size - 1) / block_size;
}

/* ---------- dirty metadata block cache ---------- */

static ext3_blkcache_t* ext3_cache_find(ext3_t* ext3, uint32_t blk) {
    ext3_blkcache_t* e = ext3->cache[blk & (EXT3_CACHE_BUCKETS - 1)];
    while(e != NULL) {
        if(e->blk == blk)
            return e;
        e = e->next;
    }
    return NULL;
}

static int32_t ext3_cache_set(ext3_t* ext3, uint32_t blk, const char* data) {
    uint32_t bucket = blk & (EXT3_CACHE_BUCKETS - 1);
    uint32_t block_size = ext3_block_size(ext3);
    ext3_blkcache_t* e = ext3_cache_find(ext3, blk);

    /* bound the transaction size: commit what is pending so far.  A
     * mid-operation commit is safe: the checkpointed partial state is
     * consistent, a crash at worst leaks an orphan inode/block that
     * e2fsck reclaims. */
    if(e == NULL && _in_commit == 0 && ext3->cache_size >= ext3->journal_txn_limit) {
        if(ext3_commit(ext3) != 0)
            return -1;
        e = ext3_cache_find(ext3, blk);
    }

    if(e == NULL) {
        e = (ext3_blkcache_t*)malloc(sizeof(ext3_blkcache_t));
        if(e == NULL)
            return -1;
        e->blk = blk;
        e->next = ext3->cache[bucket];
        ext3->cache[bucket] = e;
        ext3->cache_size++;
    }
    memcpy(e->data, data, block_size);
    /* the block is (re)written by the current transaction: an earlier
     * revoke of it in the same transaction must not suppress this new
     * copy at replay time */
    if(ext3->journal != NULL)
        jbd_cancel_revoke(ext3->journal, blk);
    return 0;
}

/* forget a block's pending journal copy.  Blocks whose old content was
 * journaled by the transaction being built must be revoked: replay must
 * not restore the old copy over whatever the block is reused for before
 * the transaction commits. */
static int32_t ext3_cache_drop(ext3_t* ext3, uint32_t blk) {
    ext3_blkcache_t** pp = &ext3->cache[blk & (EXT3_CACHE_BUCKETS - 1)];
    while(*pp != NULL) {
        ext3_blkcache_t* e = *pp;
        if(e->blk == blk) {
            *pp = e->next;
            free(e);
            ext3->cache_size--;
            if(ext3->journal != NULL)
                return jbd_revoke(ext3->journal, blk);
            return 0;
        }
        pp = &e->next;
    }
    return 0;
}

static void ext3_cache_free_all(ext3_t* ext3) {
    for(uint32_t i = 0; i < EXT3_CACHE_BUCKETS; i++) {
        ext3_blkcache_t* e = ext3->cache[i];
        while(e != NULL) {
            ext3_blkcache_t* next = e->next;
            free(e);
            e = next;
        }
        ext3->cache[i] = NULL;
    }
    ext3->cache_size = 0;
}

/* ---------- block IO routing ---------- */

/* cache-aware read: metadata written since the last commit is served
 * from the dirty cache; everything else comes from the device */
static int32_t ext3_read_blk(ext3_t* ext3, uint32_t blk, char* buf) {
    ext3_blkcache_t* e = ext3_cache_find(ext3, blk);
    if(e != NULL) {
        memcpy(buf, e->data, ext3_block_size(ext3));
        return 0;
    }
    return ext3->read_block((int32_t)blk, buf);
}

/* metadata write: journaled (cached until commit), or a plain
 * synchronous write when the filesystem has no journal */
static int32_t ext3_write_meta_blk(ext3_t* ext3, uint32_t blk, const char* buf) {
    if(ext3->journal == NULL)
        return ext3->write_block((int32_t)blk, buf);
    return ext3_cache_set(ext3, blk, buf);
}

/* file-data write (ordered mode): data goes straight to its home block,
 * only the metadata that makes it reachable is journaled */
static int32_t ext3_write_data_blk(ext3_t* ext3, uint32_t blk, const char* buf) {
    return ext3->write_block((int32_t)blk, buf);
}

static int32_t ext3_read_blocks_io(ext3_t* ext3, int32_t block, void* buf, uint32_t count) {
    char* p = (char*)buf;
    uint32_t block_size = ext3_block_size(ext3);

    if(count == 0)
        return 0;
    /* the multi-block fast path bypasses the per-block cache: only take
     * it when no pending metadata block interferes (file data never
     * does) */
    for(uint32_t i = 0; i < count; i++) {
        if(ext3_cache_find(ext3, (uint32_t)block + i) != NULL) {
            for(uint32_t j = 0; j < count; j++) {
                if(ext3_read_blk(ext3, (uint32_t)block + (uint32_t)j, p + (j * block_size)) != 0)
                    return -1;
            }
            return 0;
        }
    }
    if(ext3->read_blocks != NULL)
        return ext3->read_blocks(block, buf, count);

    for(uint32_t i = 0; i < count; i++) {
        if(ext3->read_block(block + (int32_t)i, p + (i * block_size)) != 0)
            return -1;
    }
    return 0;
}

static int32_t ext3_write_blocks_io(ext3_t* ext3, int32_t block, const void* buf, uint32_t count) {
    const char* p = (const char*)buf;
    uint32_t block_size = ext3_block_size(ext3);

    if(count == 0)
        return 0;
    if(ext3->write_blocks != NULL)
        return ext3->write_blocks(block, buf, count);

    for(uint32_t i = 0; i < count; i++) {
        if(ext3_write_data_blk(ext3, (uint32_t)block + i, p + (i * block_size)) != 0)
            return -1;
    }
    return 0;
}

/* ---------- logical block mapping ---------- */

/*
 * Returns 0 with *blk == 0 for a sparse hole (a never-allocated block,
 * e.g. created by lseek past EOF + write); -1 only for real I/O errors
 * or an out-of-range logical block. Callers must check *blk == 0.
 */
static int32_t ext3_get_data_block(ext3_t* ext3, EXT3_INODE* node, int32_t lbk, int32_t* blk) {
    uint32_t entries_per_block = ext3_indirect_entries(ext3);
    uint32_t ptr_buf[EXT3_MAX_BLOCK_SIZE / sizeof(uint32_t)];

    *blk = 0;
    if(lbk < 0)
        return -1;
    if(lbk < 12) {
        *blk = node->i_block[lbk];
        return 0;
    }
    else if(lbk < (int32_t)(entries_per_block + 12)) {
        uint32_t* indirect;
        if(node->i_block[12] == 0)
            return 0;
        indirect = ext3_get_cached_indirect_block(ext3, node->i_block[12]);
        if(indirect == NULL)
            return -1;
        *blk = (int32_t)indirect[lbk - 12];
        return 0;
    }
    else if(lbk < (int32_t)(entries_per_block * entries_per_block + entries_per_block + 12)) {
        int32_t count = lbk - 12 - (int32_t)entries_per_block;
        int32_t num = count / (int32_t)entries_per_block;
        int32_t pos_offset = count % (int32_t)entries_per_block;
        uint32_t indirect1[EXT3_MAX_BLOCK_SIZE / sizeof(uint32_t)];
        if(node->i_block[13] == 0)
            return 0;
        if(ext3_read_blk(ext3, node->i_block[13], (char*)indirect1) != 0)
            return -1;
        if(indirect1[num] == 0)
            return 0;
        if(ext3_read_blk(ext3, indirect1[num], (char*)ptr_buf) != 0)
            return -1;
        *blk = (int32_t)ptr_buf[pos_offset];
        return 0;
    }
    else if(lbk < (int32_t)(entries_per_block * entries_per_block * entries_per_block +
            entries_per_block * entries_per_block + entries_per_block + 12)) {
        int32_t count = lbk - 12 - (int32_t)entries_per_block -
            (int32_t)(entries_per_block * entries_per_block);
        int32_t num1 = count / (int32_t)(entries_per_block * entries_per_block);
        int32_t rem = count % (int32_t)(entries_per_block * entries_per_block);
        int32_t num2 = rem / (int32_t)entries_per_block;
        int32_t pos_offset = rem % (int32_t)entries_per_block;
        uint32_t indirect1[EXT3_MAX_BLOCK_SIZE / sizeof(uint32_t)];
        uint32_t indirect2[EXT3_MAX_BLOCK_SIZE / sizeof(uint32_t)];
        if(node->i_block[14] == 0)
            return 0;
        if(ext3_read_blk(ext3, node->i_block[14], (char*)indirect1) != 0)
            return -1;
        if(indirect1[num1] == 0)
            return 0;
        if(ext3_read_blk(ext3, indirect1[num1], (char*)indirect2) != 0)
            return -1;
        if(indirect2[num2] == 0)
            return 0;
        if(ext3_read_blk(ext3, indirect2[num2], (char*)ptr_buf) != 0)
            return -1;
        *blk = (int32_t)ptr_buf[pos_offset];
        return 0;
    }
    return -1;
}

static int32_t ext3_read_inode_block(ext3_t* ext3, EXT3_INODE* node, uint32_t lbk, char* buf) {
    int32_t blk = 0;
    if(ext3_get_data_block(ext3, node, (int32_t)lbk, &blk) != 0 || blk == 0)
        return -1;
    return ext3_read_blk(ext3, (uint32_t)blk, buf);
}

/* Read an indirect-pointer block, preferring the dirty deferred copy:
 * ext3_ensure_data_block defers the write-back of a single-indirect
 * block, so a raw device read on it would return stale data (wrong
 * i_blocks accounting, missed blocks on truncate/unlink). */
static int32_t ext3_read_indirect_block(ext3_t* ext3, uint32_t blk, uint32_t* buf) {
    if(blk == 0)
        return -1;
    if(_cached_indirect_blk == blk) {
        memcpy(buf, _cached_indirect_block, ext3_block_size(ext3));
        return 0;
    }
    return ext3_read_blk(ext3, blk, (char*)buf);
}

static uint32_t ext3_count_indirect_blocks(ext3_t* ext3, uint32_t blk, int32_t depth) {
    uint32_t block_size = ext3_block_size(ext3);
    uint32_t ptr_buf[EXT3_MAX_BLOCK_SIZE / sizeof(uint32_t)];
    uint32_t count = 1;
    uint32_t entries_per_block = block_size / sizeof(uint32_t);

    if(blk == 0)
        return 0;
    if(ext3_read_indirect_block(ext3, blk, ptr_buf) != 0)
        return count;

    for(uint32_t i = 0; i < entries_per_block; i++) {
        if(ptr_buf[i] == 0)
            continue;
        if(depth == 1)
            count++;
        else
            count += ext3_count_indirect_blocks(ext3, ptr_buf[i], depth - 1);
    }
    return count;
}

static uint32_t ext3_inode_reserved_sectors(ext3_t* ext3, const EXT3_INODE* node) {
    uint32_t blocks = 0;
    uint32_t sectors_per_block = ext3_block_size(ext3) / SECTOR_SIZE;

    for(int32_t i = 0; i < 12; i++) {
        if(node->i_block[i] != 0)
            blocks++;
    }
    blocks += ext3_count_indirect_blocks(ext3, node->i_block[12], 1);
    blocks += ext3_count_indirect_blocks(ext3, node->i_block[13], 2);
    blocks += ext3_count_indirect_blocks(ext3, node->i_block[14], 3);
    return blocks * sectors_per_block;
}

static int32_t ext3_free_indirect_tree(ext3_t* ext3, uint32_t blk, int32_t depth) {
    uint32_t block_size = ext3_block_size(ext3);
    uint32_t ptr_buf[EXT3_MAX_BLOCK_SIZE / sizeof(uint32_t)];
    uint32_t entries_per_block = block_size / sizeof(uint32_t);

    if(blk == 0)
        return 0;
    if(ext3_read_indirect_block(ext3, blk, ptr_buf) != 0)
        return -1;

    for(uint32_t i = 0; i < entries_per_block; i++) {
        if(ptr_buf[i] == 0)
            continue;
        if(depth == 1) {
            if(ext3_bdealloc(ext3, ptr_buf[i]) != 0)
                return -1;
        }
        else if(ext3_free_indirect_tree(ext3, ptr_buf[i], depth - 1) != 0) {
            return -1;
        }
    }
    return ext3_bdealloc(ext3, blk);
}

static int32_t ext3_free_inode_data(ext3_t* ext3, EXT3_INODE* node) {
    for(int32_t i = 0; i < 12; i++) {
        if(node->i_block[i] == 0)
            continue;
        if(ext3_bdealloc(ext3, node->i_block[i]) != 0)
            return -1;
        node->i_block[i] = 0;
    }
    if(ext3_free_indirect_tree(ext3, node->i_block[12], 1) != 0)
        return -1;
    if(ext3_free_indirect_tree(ext3, node->i_block[13], 2) != 0)
        return -1;
    if(ext3_free_indirect_tree(ext3, node->i_block[14], 3) != 0)
        return -1;
    node->i_block[12] = 0;
    node->i_block[13] = 0;
    node->i_block[14] = 0;
    node->i_size = 0;
    node->i_blocks = 0;
    return 0;
}

static int32_t ext3_dir_is_empty(ext3_t* ext3, EXT3_INODE* node) {
    uint32_t block_size = ext3_block_size(ext3);
    uint32_t blocks = ext3_dir_block_count(ext3, node);
    char buf[EXT3_MAX_BLOCK_SIZE];

    for(uint32_t lbk = 0; lbk < blocks; lbk++) {
        char* cp = buf;
        if(ext3_read_inode_block(ext3, node, lbk, buf) != 0)
            return -1;
        while(cp < (buf + block_size)) {
            EXT3_DIR_T* dp = (EXT3_DIR_T*)cp;
            if(dp->name_len == 0 || dp->rec_len < 12 || dp->rec_len < need_len(dp->name_len) ||
                    (cp + dp->rec_len) > (buf + block_size)) {
                break;
            }
            if(dp->inode != 0 &&
                    !ext3_dirent_name_equals(dp, ".") &&
                    !ext3_dirent_name_equals(dp, "..")) {
                return 0;
            }
            cp += dp->rec_len;
        }
    }
    return 1;
}

/*test bit is on or off*/
static inline int32_t tst_bit(char *buf, int32_t bit) {
    int32_t i, j;
    i = bit / 8;
    j = bit % 8;
    if (buf[i] & (1 << j))
        return 1;
    return 0;
}

/*set bit off*/
static inline int32_t clr_bit(char *buf, int32_t bit) {
    int32_t i, j;
    i = bit / 8;
    j = bit % 8;
    buf[i] &= ~(1 << j);
    return 0;
}

/*set bit on*/
static inline int32_t set_bit(char *buf, int32_t bit) {
    int32_t i, j;
    i = bit / 8;
    j = bit % 8;
    buf[i] |= (1 << j);
    return 0;
}

/*get group descriptor index by inode*/
static inline uint32_t get_gd_index_by_ino(ext3_t* ext3, uint32_t ino) {
    return ((ino -1) / ext3->super.s_inodes_per_group);
}

/*get group descriptor index by block*/
static inline uint32_t get_gd_index_by_block(ext3_t* ext3, uint32_t block) {
    return ((block - ext3->super.s_first_data_block) / ext3->super.s_blocks_per_group);
}

/*get inode index in group*/
static inline uint32_t get_ino_in_group(ext3_t* ext3, uint32_t ino, uint32_t index) {
    return ino - (index * ext3->super.s_inodes_per_group);
}

/*get block index in group*/
static inline uint32_t get_block_in_group(ext3_t* ext3, uint32_t block, uint32_t index) {
    return block - ext3_group_start_block(ext3, index);
}

/*write back group descriptor block holding descriptor `index`*/
static int32_t set_gd(ext3_t* ext3, uint32_t index) {
    uint32_t gd_size = sizeof(EXT3_GD);
    uint32_t block_size = ext3_block_size(ext3);
    uint32_t gd_num = (block_size / gd_size);
    char buf[EXT3_MAX_BLOCK_SIZE];

    uint32_t blk_index = (index / gd_num);
    uint32_t first = blk_index * gd_num;
    uint32_t count = ext3->group_num - first;

    if(count > gd_num)
        count = gd_num;
    memset(buf, 0, block_size);
    memcpy(buf, &ext3->gds[first], count * gd_size);
    return ext3_write_meta_blk(ext3, ext3_gdt_start_block(ext3) + blk_index, buf);
}

/*write back super block (read-modify-write: the super shares its block
 * with other data when block size > 1024)*/
static int32_t ext3_write_super_block(ext3_t* ext3) {
    uint32_t block_size = ext3_block_size(ext3);
    char buf[EXT3_MAX_BLOCK_SIZE];
    int32_t super_block = (block_size == EXT3_MIN_BLOCK_SIZE) ? 1 : 0;

    if(ext3_read_blk(ext3, (uint32_t)super_block, buf) != 0)
        return -1;
    if(block_size == EXT3_MIN_BLOCK_SIZE)
        memcpy(buf, &ext3->super, sizeof(EXT3_SUPER));
    else
        memcpy(buf + EXT3_MIN_BLOCK_SIZE, &ext3->super, sizeof(EXT3_SUPER));
    return ext3_write_meta_blk(ext3, (uint32_t)super_block, buf);
}

static void mark_gd_dirty(ext3_t* ext3, uint32_t index) {
        if(ext3->dirty_gds != NULL && index < (uint32_t)ext3->group_num)
                ext3->dirty_gds[index] = 1;
}

static int32_t ext3_flush_meta(ext3_t* ext3) {
        if(_cached_indirect_blk != 0 && _cached_indirect_dirty != 0) {
                if(ext3_write_meta_blk(ext3, _cached_indirect_blk, (char*)_cached_indirect_block) != 0)
                        return -1;
                _cached_indirect_dirty = 0;
        }

        if(_cached_block_bitmap != NULL &&
                        _cached_block_bitmap_blk != 0 &&
                        _cached_block_bitmap_dirty != 0) {
                if(ext3_write_meta_blk(ext3, _cached_block_bitmap_blk, _cached_block_bitmap) != 0)
                        return -1;
                _cached_block_bitmap_dirty = 0;
        }

        if(_cached_inode_bitmap != NULL &&
                        _cached_inode_bitmap_blk != 0 &&
                        _cached_inode_bitmap_dirty != 0) {
                if(ext3_write_meta_blk(ext3, _cached_inode_bitmap_blk, _cached_inode_bitmap) != 0)
                        return -1;
                _cached_inode_bitmap_dirty = 0;
        }

        if(ext3->dirty_gds != NULL) {
                for(uint32_t i = 0; i < (uint32_t)ext3->group_num; i++) {
                        if(ext3->dirty_gds[i] != 0) {
                                if(set_gd(ext3, i) != 0)
                                        return -1;
                                ext3->dirty_gds[i] = 0;
                        }
                }
        }

        if(ext3->dirty_super != 0) {
                if(ext3_write_super_block(ext3) != 0)
                        return -1;
                ext3->dirty_super = 0;
        }
        return 0;
}

static char* ext3_get_block_bitmap(ext3_t* ext3, uint32_t blk) {
        if(_cached_block_bitmap == NULL)
                return NULL;
        if(_cached_block_bitmap_blk != blk) {
                if(_cached_block_bitmap_blk != 0 && _cached_block_bitmap_dirty != 0) {
                        if(ext3_write_meta_blk(ext3, _cached_block_bitmap_blk, _cached_block_bitmap) != 0)
                                return NULL;
                        _cached_block_bitmap_dirty = 0;
                }
                if(ext3_read_blk(ext3, blk, _cached_block_bitmap) != 0)
                        return NULL;
                _cached_block_bitmap_blk = blk;
        }
        return _cached_block_bitmap;
}

static char* ext3_get_inode_bitmap(ext3_t* ext3, uint32_t blk) {
        if(_cached_inode_bitmap == NULL)
                return NULL;
        if(_cached_inode_bitmap_blk != blk) {
                if(_cached_inode_bitmap_blk != 0 && _cached_inode_bitmap_dirty != 0) {
                        if(ext3_write_meta_blk(ext3, _cached_inode_bitmap_blk, _cached_inode_bitmap) != 0)
                                return NULL;
                        _cached_inode_bitmap_dirty = 0;
                }
                if(ext3_read_blk(ext3, blk, _cached_inode_bitmap) != 0)
                        return NULL;
                _cached_inode_bitmap_blk = blk;
        }
        return _cached_inode_bitmap;
}

static uint32_t* ext3_get_cached_indirect_block(ext3_t* ext3, uint32_t blk) {
        if(blk == 0)
                return NULL;

        if(_cached_indirect_blk != blk) {
                if(_cached_indirect_blk != 0 && _cached_indirect_dirty != 0) {
                        if(ext3_write_meta_blk(ext3, _cached_indirect_blk, (char*)_cached_indirect_block) != 0)
                                return NULL;
                        _cached_indirect_dirty = 0;
                }
                if(ext3_read_blk(ext3, blk, (char*)_cached_indirect_block) != 0)
                        return NULL;
                _cached_indirect_blk = blk;
        }
        return _cached_indirect_block;
}

static void inc_free_blocks(ext3_t* ext3, uint32_t block) {
    int32_t index = get_gd_index_by_block(ext3, block);
    ext3->gds[index].bg_free_blocks_count++;
        mark_gd_dirty(ext3, (uint32_t)index);

    ext3->super.s_free_blocks_count++;
        ext3->dirty_super = 1;
}

static void inc_free_inodes(ext3_t* ext3, uint32_t ino) {
    uint32_t index = get_gd_index_by_ino(ext3, ino);
    ext3->gds[index].bg_free_inodes_count++;
        mark_gd_dirty(ext3, index);

    ext3->super.s_free_inodes_count++;
        ext3->dirty_super = 1;
}

static void dec_free_blocks(ext3_t* ext3, uint32_t block) {
    uint32_t index = get_gd_index_by_block(ext3, block);
    ext3->gds[index].bg_free_blocks_count--;
        mark_gd_dirty(ext3, index);

    ext3->super.s_free_blocks_count--;
        ext3->dirty_super = 1;
}

static void dec_free_inodes(ext3_t* ext3, uint32_t ino) {
    uint32_t index = get_gd_index_by_ino(ext3, ino);
    ext3->gds[index].bg_free_inodes_count--;
        mark_gd_dirty(ext3, index);

    ext3->super.s_free_inodes_count--;
        ext3->dirty_super = 1;
}

static int32_t ext3_idealloc(ext3_t* ext3, uint32_t ino) {
    if (ino > ext3->super.s_inodes_count)
        return -1;

    // get inode bitmap block
    uint32_t index = get_gd_index_by_ino(ext3, ino);
    uint32_t ino_g = get_ino_in_group(ext3, ino, index);

    uint32_t blk = ext3->gds[index].bg_inode_bitmap;
        char* buf = ext3_get_inode_bitmap(ext3, blk);
        if(buf == NULL)
        return -1;

    clr_bit(buf, (int32_t)(ino_g - 1));
        _cached_inode_bitmap_dirty = 1;
    inc_free_inodes(ext3, ino);
    return 0;
}

static int32_t ext3_bdealloc(ext3_t* ext3, uint32_t block) {
    if(block == 0)
        return -1;

    if(_cached_indirect_blk == block) {
        _cached_indirect_blk = 0;
        _cached_indirect_dirty = 0;
        memset(_cached_indirect_block, 0, sizeof(_cached_indirect_block));
    }

        /* drop a pending journal copy of the block and revoke it so the
         * current transaction's replay cannot restore the old content
         * over whatever the block is reused for */
        if(ext3_cache_drop(ext3, block) != 0)
                return -1;

    uint32_t index = get_gd_index_by_block(ext3, block);
    uint32_t block_g = get_block_in_group(ext3, block, index);

    uint32_t blk = ext3->gds[index].bg_block_bitmap;
        char* buf = ext3_get_block_bitmap(ext3, blk);
        if(buf == NULL)
        return -1;

    clr_bit(buf, (int32_t)block_g);
        _cached_block_bitmap_dirty = 1;
    // update free block count in EXT3_SUPER and EXT3_GD
    inc_free_blocks(ext3, block);

    /* Do NOT zero the freed block on disk: unlinking/truncating a large
     * file would issue one synchronous SD write (plus a read-back verify
     * on real hosts) per block, which pushed rm of a multi-MB file past
     * the kernel's 10s IPC timeout and got the unlink aborted mid-flight.
     * Freed blocks may now hold stale data; every path that reads a
     * freshly allocated block before writing it must not assume zeroes
     * (ext3_write memsets partial-write targets, dir/indirect blocks are
     * always initialized in memory before their first write). */
    return 0;
}

static uint32_t ext3_ialloc(ext3_t* ext3) {  //alloc a node, inode start with 1 not 0!!
    uint32_t index = 0;
    uint32_t i, blk = 0, ino = 0;
    for (i=0; i < ext3->super.s_inodes_count; i++){
        ino = i + 1;
        if((i % ext3->super.s_inodes_per_group) == 0) {
            index = get_gd_index_by_ino(ext3, ino);
            blk = ext3->gds[index].bg_inode_bitmap;
                        if(ext3_get_inode_bitmap(ext3, blk) == NULL)
                return 0;
        }

        uint32_t ino_g = get_ino_in_group(ext3, ino, index);
                        char* buf = _cached_inode_bitmap;
        if (tst_bit(buf, ino_g-1) == 0){
            set_bit(buf, (int32_t)(ino_g - 1));
                        _cached_inode_bitmap_dirty = 1;
            // update free inode count in EXT3_SUPER and EXT3_GD
            dec_free_inodes(ext3, ino);
            return ino;
        }
    }
    return 0;
}

static int32_t ext3_balloc(ext3_t* ext3) { //alloc a block
    uint32_t index = 0;
    uint32_t blk = 0;

    uint32_t start = ext3->next_alloc_block;
    if(start < ext3->super.s_first_data_block || start >= ext3->super.s_blocks_count)
        start = ext3->super.s_first_data_block;

    for(uint32_t pass = 0; pass < 2; pass++) {
        uint32_t begin = (pass == 0) ? start : ext3->super.s_first_data_block;
        uint32_t end = (pass == 0) ? ext3->super.s_blocks_count : start;

        for(uint32_t block = begin; block < end; block++) {
            if(((block - ext3->super.s_first_data_block) % ext3->super.s_blocks_per_group) == 0) {
                index = get_gd_index_by_block(ext3, block);
                blk = ext3->gds[index].bg_block_bitmap;
                                if(ext3_get_block_bitmap(ext3, blk) == NULL)
                    return 0;
            }

            uint32_t block_g = get_block_in_group(ext3, block, index);
                        char* buf = _cached_block_bitmap;
            if(tst_bit(buf, (int32_t)block_g) == 0) {
                set_bit(buf, (int32_t)block_g);
                                _cached_block_bitmap_dirty = 1;
                dec_free_blocks(ext3, block);
                ext3->next_alloc_block = block + 1;
                if(ext3->next_alloc_block >= ext3->super.s_blocks_count)
                    ext3->next_alloc_block = ext3->super.s_first_data_block;
                return block;
            }
        }
    }
    return 0;
}

static int32_t need_len(int32_t len) {
    return 4 * ((8 + len + 3) / 4);
}

static int32_t write_child(ext3_t* ext3, EXT3_INODE* pip, uint32_t ino, const char *name, int32_t ftype) {
    int32_t nlen, ideal_len, remain, blk;
    uint32_t now = ext3_now();
    uint32_t block_size = ext3_block_size(ext3);
    uint32_t name_len = strlen(name);
    uint32_t blocks = ext3_dir_block_count(ext3, pip);
    char buf[EXT3_MAX_BLOCK_SIZE];
    char *cp;
    EXT3_DIR_T *dp = NULL;
    //(1)-(3)
    nlen = need_len((int32_t)name_len);
    for(uint32_t lbk = 0; ; lbk++) {
        if(lbk >= blocks) {
            if(ext3_ensure_data_block(ext3, pip, (int32_t)lbk, &blk, NULL) != 0)
                return -1;
            memset(buf, 0, block_size);
            dp = (EXT3_DIR_T *)buf;
            dp->inode = ino;
            dp->rec_len = (uint16_t)block_size;
            dp->name_len = (uint8_t)name_len;
            dp->file_type = (uint8_t)ftype;
            strcpy(dp->name, name);
            pip->i_size = (lbk + 1) * block_size;
            pip->i_blocks = ext3_inode_reserved_sectors(ext3, pip);
            pip->i_mtime = now;
            pip->i_ctime = now;
            /* directory blocks are metadata: journaled */
            return ext3_write_meta_blk(ext3, (uint32_t)blk, buf);
        }

        if(ext3_get_data_block(ext3, pip, (int32_t)lbk, &blk) != 0 || blk == 0)
            return -1;
        if(ext3_read_blk(ext3, (uint32_t)blk, buf) != 0)
            return -1;
        cp = buf;
        dp = (EXT3_DIR_T *)cp;
        if(dp->inode == 0) {
            dp->inode = ino;
            dp->rec_len = (uint16_t)block_size;
            dp->name_len = (uint8_t)name_len;
            dp->file_type = (uint8_t)ftype;
            strcpy(dp->name, name);
            pip->i_mtime = now;
            pip->i_ctime = now;
            return ext3_write_meta_blk(ext3, (uint32_t)blk, buf);
        }

        //(4) get last entry in block
        //NOTE: a torn/partial sector write (e.g. power loss or a failed
        //sd write) can leave rec_len==0 in the middle of the block, which
        //would spin this walk forever. Stop at the corrupted entry and
        //self-heal by extending it to cover the rest of the block.
        while((cp + dp->rec_len) < (buf + block_size)) {
            if(dp->rec_len == 0 || dp->rec_len < need_len(dp->name_len))
                break;
            cp += dp->rec_len;
            dp = (EXT3_DIR_T *)cp;
        }
        if(dp->rec_len == 0 || dp->rec_len < need_len(dp->name_len) || (cp + dp->rec_len) > (buf + block_size))
            dp->rec_len = (uint16_t)((buf + block_size) - cp);
        ideal_len = need_len(dp->name_len);
        remain = dp->rec_len-ideal_len;
        if(remain >= nlen){
            dp->rec_len = ideal_len;
            cp += dp->rec_len;
            dp = (EXT3_DIR_T *)cp;
            dp->inode = ino;
            dp->rec_len = remain;
            dp->name_len = (uint8_t)name_len;
            dp->file_type = (uint8_t)ftype;
            strcpy(dp->name, name);
            pip->i_mtime = now;
            pip->i_ctime = now;
            return ext3_write_meta_blk(ext3, (uint32_t)blk, buf);
        }
    }
    return -1;
}

static int32_t ext3_rm_child(ext3_t* ext3, EXT3_INODE *ip, const char *name) {
    int32_t rec_len, found, first_len, blk;
    char *cp = NULL, *precp = NULL;
    EXT3_DIR_T *dp = NULL;
    uint32_t block_size = ext3_block_size(ext3);
    uint32_t blocks = ext3_dir_block_count(ext3, ip);
    char buf[EXT3_MAX_BLOCK_SIZE];

    found = 0;
    rec_len = 0;
    first_len = 0;
    for(uint32_t lbk = 0; lbk < blocks; lbk++) {
        if(ext3_get_data_block(ext3, ip, (int32_t)lbk, &blk) != 0 || blk == 0)
            return -1;
        if(ext3_read_blk(ext3, (uint32_t)blk, buf) != 0)
            return -1;
        cp = buf;
        precp = NULL;
        while(cp < (buf + block_size)) {
            dp = (EXT3_DIR_T *)cp;
            if(dp->name_len == 0 || dp->rec_len < 12 || dp->rec_len < need_len(dp->name_len) ||
                    (cp + dp->rec_len) > (buf + block_size))
                break;
            if(found == 0 && dp->inode != 0 && ext3_dirent_name_equals(dp, name)) {
                //(2).1. if (first and only entry in a data block){
                if(dp->rec_len == block_size){
                    dp->inode = 0;
                    ip->i_mtime = ext3_now();
                    ip->i_ctime = ip->i_mtime;
                    return ext3_write_meta_blk(ext3, (uint32_t)blk, buf);
                }
                //(2).2. else if LAST entry in block
                else if(precp != NULL && (cp + dp->rec_len) == (buf + block_size)){
                    dp = (EXT3_DIR_T *)precp;
                    dp->rec_len = (uint16_t)(block_size - (precp - buf));
                    ip->i_mtime = ext3_now();
                    ip->i_ctime = ip->i_mtime;
                    return ext3_write_meta_blk(ext3, (uint32_t)blk, buf);
                }
                //(2).3. else: entry is first but not the only entry or in the middle of a block:
                else{
                    found = 1;
                    rec_len = dp->rec_len;
                    first_len = cp-buf;
                }
            }
            if(found == 0)
                precp = cp;
            cp += dp->rec_len;
        }

        if(found == 1) {
            // Shift remaining entries left
            char cpbuf[EXT3_MAX_BLOCK_SIZE];
            memset(cpbuf, 0, block_size);
            memcpy(cpbuf, buf, first_len);
            memcpy(cpbuf + first_len, buf + first_len + rec_len, block_size - (first_len + rec_len));
            // Update the last entry's rec_len
            cp = cpbuf;
            while((cp + ((EXT3_DIR_T *)cp)->rec_len) < (cpbuf + block_size)) {
                if(((EXT3_DIR_T *)cp)->rec_len == 0) //corrupted entry, heal below
                    break;
                cp += ((EXT3_DIR_T *)cp)->rec_len;
            }
            ((EXT3_DIR_T *)cp)->rec_len = (uint16_t)(block_size - (cp - cpbuf));
            ip->i_mtime = ext3_now();
            ip->i_ctime = ip->i_mtime;
            return ext3_write_meta_blk(ext3, (uint32_t)blk, cpbuf);
        }
    }
    return -1;
}
int32_t ext3_read_block(ext3_t* ext3, EXT3_INODE* node, char *buf, int32_t nbytes, int32_t offset) {
    //(2) count = 0
    // avil = fileSize - OFT's offset // number of bytes still available in file.
    int32_t count_read = 0;
    char *cq = buf;
    int32_t avil = node->i_size - offset;
    int32_t blk =0, lbk = 0, start_byte = 0, remain = 0;
    uint32_t block_size = ext3_block_size(ext3);
    //(3)
    /*(4) Compute LOGICAL BLOCK number lbk and start_byte in that block from offset;
        lbk       = oftp->offset / EXT3_BLOCK_SIZE;
        start_byte = oftp->offset % EXT3_BLOCK_SIZE;*/
    lbk = offset / (int32_t)block_size;
    start_byte = offset % (int32_t)block_size;
    if(nbytes > ((int32_t)block_size - start_byte))
        nbytes = (int32_t)block_size - start_byte;
    //(5) READ
    if(ext3_get_data_block(ext3, node, lbk, &blk) != 0)
        return -1;

    char readbuf[EXT3_MAX_BLOCK_SIZE];
    char *cp;
    if(blk == 0) {
        /* Sparse hole (lseek past EOF + write): reads as zeroes instead
         * of failing, otherwise the hole would look like EOF and hide
         * all data written beyond it. */
        memset(readbuf, 0, block_size);
        cp = readbuf + start_byte;
    }
    /* reads go through the cache-aware path: directory blocks are
     * journaled metadata, a raw device read could be stale */
    else if(start_byte == 0 && nbytes >= (int32_t)block_size && avil >= (int32_t)block_size) {
        /* whole-block copy straight into the caller buffer: only when
         * the block is entirely within the file (avil).  With a short
         * tail (avil < block_size) the direct copy would fill the
         * caller's buffer with a full block even though only avil
         * bytes are returned, overflowing callers that size their
         * buffer to the requested length. */
        if(ext3_read_blk(ext3, (uint32_t)blk, cq) != 0)
            return -1;
        cp = cq;
    }
    else {
        if(ext3_read_blk(ext3, (uint32_t)blk, readbuf) != 0)
            return -1;
        cp = readbuf + start_byte;
    }
    remain = (int32_t)block_size - start_byte;
    //(6)
    while(remain){
        int32_t min = 0;
        if(avil <= nbytes){
            min = avil;
        }
        else{
            min = nbytes;
        }
        if(cp != cq)
            memcpy(cq, cp, min);
        offset += min;
        count_read += min;
        avil -= min;
        nbytes -= min;
        remain -= min;
        if(nbytes == 0 || avil == 0){
            break;
        }
    }
    return count_read;
}

int32_t ext3_read(ext3_t* ext3, EXT3_INODE* node, char *buf, int32_t nbytes, int32_t offset) {
    char* p = buf;
    int32_t ret = nbytes;
    int32_t avil = node->i_size - offset;
    uint32_t block_size = ext3_block_size(ext3);
    while(nbytes > 0) {
        if(offset >= (int32_t)node->i_size)
            break;
        if((offset % (int32_t)block_size) == 0 && nbytes >= (int32_t)block_size && avil >= (int32_t)block_size) {
            int32_t start_lbk = offset / (int32_t)block_size;
            int32_t first_blk = 0;
            int32_t full_blocks = avil / (int32_t)block_size;
            int32_t max_blocks = nbytes / (int32_t)block_size;

            if(max_blocks > full_blocks)
                max_blocks = full_blocks;
            if(max_blocks > 0 && ext3_get_data_block(ext3, node, start_lbk, &first_blk) == 0 &&
                    first_blk != 0) {
                int32_t blocks = 1;
                while(blocks < max_blocks) {
                    int32_t next_blk = 0;
                    if(ext3_get_data_block(ext3, node, start_lbk + blocks, &next_blk) != 0)
                        break;
                    if(next_blk != (first_blk + blocks))
                        break;
                    blocks++;
                }

                if(ext3_read_blocks_io(ext3, first_blk, p, (uint32_t)blocks) != 0)
                    return ret - nbytes;

                int32_t rd = blocks * (int32_t)block_size;
                nbytes -= rd;
                offset += rd;
                avil -= rd;
                p += rd;
                continue;
            }
        }

        int32_t rd = ext3_read_block(ext3, node, p, nbytes, offset);
        if(rd <= 0)
            return ret - nbytes;
        nbytes -= rd;
        offset += rd;
        avil -= rd;
        p += rd;
    }
    return ret - nbytes;
}

static EXT3_INODE* get_node_by_ino(ext3_t* ext3, uint32_t ino, char* buf) {
    if(ino == 0 || ino > ext3->super.s_inodes_count)
        return NULL;
    uint32_t bgid = get_gd_index_by_ino(ext3, ino);
    ino = get_ino_in_group(ext3, ino, bgid);
    uint32_t inode_size = ext3_inode_size(ext3);
    uint32_t inodes_per_block = ext3_block_size(ext3) / inode_size;
    uint32_t offset = (ino - 1) % inodes_per_block;

    /* inode table blocks are journaled metadata: cache-aware read */
    uint32_t blk = ext3->gds[bgid].bg_inode_table + ((ino - 1) / inodes_per_block);
    if(ext3_read_blk(ext3, blk, buf) != 0)
        return NULL;
    return (EXT3_INODE *)(buf + (offset * inode_size));
}

int32_t ext3_put_node(ext3_t* ext3, uint32_t ino, EXT3_INODE *node) {
    uint32_t bgid = get_gd_index_by_ino(ext3, ino);
    ino = get_ino_in_group(ext3, ino, bgid);
    uint32_t inode_size = ext3_inode_size(ext3);
    uint32_t inodes_per_block = ext3_block_size(ext3) / inode_size;
    uint32_t offset = (ino - 1) % inodes_per_block;
    uint32_t blk = ext3->gds[bgid].bg_inode_table + ((ino - 1) / inodes_per_block);
    char buf[EXT3_MAX_BLOCK_SIZE];
    /* read-modify-write through the journal path so the whole inode
     * table block is committed atomically */
    if(ext3_read_blk(ext3, blk, buf) != 0)
        return -1;
    memcpy(buf + (offset * inode_size), node, sizeof(EXT3_INODE));
    if(ext3_write_meta_blk(ext3, blk, buf) != 0)
        return -1;
    return ext3_flush_meta(ext3);
}

int32_t ext3_create_dir(ext3_t* ext3, uint32_t father_ino, EXT3_INODE* father_inp, const char *name,
        uint16_t uid, uint16_t gid, uint16_t mode ) {
    uint32_t ino, i, blk;
    uint32_t block_size = ext3_block_size(ext3);
    uint32_t now = ext3_now();
    EXT3_INODE* inp;
    char buf[EXT3_MAX_BLOCK_SIZE];

    ino = ext3_ialloc(ext3); //alloc a node id from table
    if(ino == 0)
        return -1;
    blk = ext3_balloc(ext3); //alloc a block
    if(blk == 0) //disk full: nothing committed to the parent dir yet, roll back
        goto fail_ino;

    inp = get_node_by_ino(ext3, ino, buf); //read inode from block
    if(inp == NULL)
        goto fail;
    //set inode info
    /* Force the on-disk file-type bits: the VFS only hands us the
     * permission bits, and without S_IFDIR every other ext2/ext3
     * implementation (Linux/macOS/fsck) treats the inode as garbage. */
    inp->i_mode = EXT3_S_IFDIR | (mode & 0x0FFF);
    inp->i_uid  = uid;
    inp->i_gid  = gid;
    inp->i_size = block_size;	        // Size in bytes (one block for dir)
    inp->i_links_count = 2;	  // "." plus the entry in the parent dir
    inp->i_atime = now;
    inp->i_ctime = now;
    inp->i_mtime = now;
    inp->i_dtime = 0;
    inp->i_blocks = block_size / SECTOR_SIZE; // # of 512-byte blocks reserved for this inode
    inp->i_block[0] = blk;
    for(i=1; i<15; i++){
        inp->i_block[i] = 0;
    }

    if(ext3_put_node(ext3, ino, inp) != 0)
        goto fail; //write inode back to block

    /* Initialize the new directory's data block ON DISK with "." and
     * "..". The old code left the freshly allocated block holding
     * whatever bytes it previously had: the live directory tree is
     * served from vfsd's RAM nodes so everything looked fine, but
     * after a reboot the tree rebuild walked this garbage block and
     * every child created inside the directory was lost/hidden. */
    memset(buf, 0, block_size);
    EXT3_DIR_T* dp = (EXT3_DIR_T*)buf;
    dp->inode = ino;
    dp->rec_len = 12;
    dp->name_len = 1;
    dp->file_type = EXT3_FT_DIR;
    dp->name[0] = '.';
    dp = (EXT3_DIR_T*)(buf + 12);
    dp->inode = father_ino;
    dp->rec_len = (uint16_t)(block_size - 12);
    dp->name_len = 2;
    dp->file_type = EXT3_FT_DIR;
    dp->name[0] = '.';
    dp->name[1] = '.';
    /* directory blocks are journaled metadata */
    if(ext3_write_meta_blk(ext3, blk, buf) != 0)
        goto fail;

    if(write_child(ext3, father_inp, ino, name, EXT3_FT_DIR) < 0) //write dir info (name, type)
        goto fail;

    /* write_child may have grown the parent (new i_block/i_size) and
     * ".." adds a link to it: persist the parent inode, otherwise the
     * changes only live in the caller's in-memory copy. */
    father_inp->i_links_count++;
    father_inp->i_mtime = now;
    father_inp->i_ctime = now;
    /* keep bg_used_dirs_count in sync so external tools (fsck) stay happy */
    uint32_t gidx = get_gd_index_by_ino(ext3, ino);
    ext3->gds[gidx].bg_used_dirs_count++;
    mark_gd_dirty(ext3, gidx);
    if(ext3_put_node(ext3, father_ino, father_inp) != 0)
        return -1; //the dirent is already committed; only the parent's inode update was lost
    return ino;

fail: //no dirent written yet: release both allocations
    ext3_bdealloc(ext3, blk);
fail_ino:
    ext3_idealloc(ext3, ino);
    ext3_flush_meta(ext3);
    return -1;
}

int32_t ext3_create_file(ext3_t* ext3, uint32_t father_ino, EXT3_INODE* father_inp, const char *name,
        uint16_t uid, uint16_t gid, uint16_t mode ) {
    uint32_t ino, i;
    uint32_t now = ext3_now();
    EXT3_INODE* inp;
    char buf[EXT3_MAX_BLOCK_SIZE];

    ino = ext3_ialloc(ext3);
    if(ino == 0)
        return -1;

    inp = get_node_by_ino(ext3, ino, buf);
    if(inp == NULL)
        goto fail;
    inp->i_mode = EXT3_S_IFREG | (mode & 0x0FFF);
    inp->i_uid  = uid;
    inp->i_gid  = gid;
    inp->i_size = 0;	        // Size in bytes
    inp->i_links_count = 1;	  // the entry in the parent dir
    inp->i_atime = now;
    inp->i_ctime = now;
    inp->i_mtime = now;
    inp->i_dtime = 0;
    inp->i_blocks = 0;        // # of 512-byte blocks reserved for this inode
    for(i=0; i<15; i++){
        inp->i_block[i] = 0;
    }
    if(ext3_put_node(ext3, ino, inp) != 0)
        goto fail;

    if(write_child(ext3, father_inp, ino, name, EXT3_FT_FILE) < 0)
        goto fail;

    /* Persist parent inode changes made by write_child (see above). */
    father_inp->i_mtime = now;
    father_inp->i_ctime = now;
    if(ext3_put_node(ext3, father_ino, father_inp) != 0)
        return -1; //the dirent is already committed; only the parent's inode update was lost
    return ino;

fail: //no dirent written yet: release the allocated inode
    ext3_idealloc(ext3, ino);
    ext3_flush_meta(ext3);
    return -1;
}

static int32_t ext3_ensure_data_block(ext3_t* ext3, EXT3_INODE* node, int32_t lbk, int32_t* blk, int32_t* is_new) {
    uint32_t entries_per_block = ext3_indirect_entries(ext3);
    uint32_t indirect1[EXT3_MAX_BLOCK_SIZE / sizeof(uint32_t)];
    uint32_t indirect2[EXT3_MAX_BLOCK_SIZE / sizeof(uint32_t)];
    uint32_t indirect3[EXT3_MAX_BLOCK_SIZE / sizeof(uint32_t)];
    uint32_t block_size = ext3_block_size(ext3);
    uint32_t sectors_per_block = block_size / SECTOR_SIZE;

    if(is_new != NULL)
        *is_new = 0;

    if(lbk < 12) {
        if(node->i_block[lbk] == 0) {
            node->i_block[lbk] = ext3_balloc(ext3);
            if(node->i_block[lbk] == 0)
                return -1;
            node->i_blocks += sectors_per_block;
            if(is_new != NULL)
                *is_new = 1;
        }
        *blk = node->i_block[lbk];
        return 0;
    }

    if(lbk < (int32_t)(entries_per_block + 12)) {
        uint32_t* indirect;
        if(node->i_block[12] == 0) {
            node->i_block[12] = ext3_balloc(ext3);
            if(node->i_block[12] == 0)
                return -1;
            node->i_blocks += sectors_per_block;
            if(_cached_indirect_blk != node->i_block[12]) {
                if(_cached_indirect_blk != 0 && _cached_indirect_dirty != 0) {
                    if(ext3_write_meta_blk(ext3, _cached_indirect_blk, (char*)_cached_indirect_block) != 0)
                        return -1;
                }
                _cached_indirect_blk = node->i_block[12];
                _cached_indirect_dirty = 0;
            }
            memset(_cached_indirect_block, 0, block_size);
        }
        indirect = ext3_get_cached_indirect_block(ext3, node->i_block[12]);
        if(indirect == NULL)
            return -1;
        if(indirect[lbk - 12] == 0) {
            indirect[lbk - 12] = ext3_balloc(ext3);
            if(indirect[lbk - 12] == 0)
                return -1;
            node->i_blocks += sectors_per_block;
            _cached_indirect_dirty = 1;
            if(is_new != NULL)
                *is_new = 1;
        }
        *blk = (int32_t)indirect[lbk - 12];
        return 0;
    }

    if(lbk < (int32_t)(entries_per_block * entries_per_block + entries_per_block + 12)) {
        int32_t count = lbk - 12 - (int32_t)entries_per_block;
        int32_t num = count / (int32_t)entries_per_block;
        int32_t pos = count % (int32_t)entries_per_block;

        /* single-indirect blocks are metadata: journaled writes,
         * cache-aware reads */
        if(node->i_block[13] == 0) {
            node->i_block[13] = ext3_balloc(ext3);
            if(node->i_block[13] == 0)
                return -1;
            node->i_blocks += sectors_per_block;
            memset(indirect1, 0, block_size);
            if(ext3_write_meta_blk(ext3, node->i_block[13], (char*)indirect1) != 0)
                return -1;
        }
        if(ext3_read_blk(ext3, node->i_block[13], (char*)indirect1) != 0)
            return -1;
        if(indirect1[num] == 0) {
            indirect1[num] = ext3_balloc(ext3);
            if(indirect1[num] == 0)
                return -1;
            node->i_blocks += sectors_per_block;
            memset(indirect2, 0, block_size);
            if(ext3_write_meta_blk(ext3, indirect1[num], (char*)indirect2) != 0)
                return -1;
            if(ext3_write_meta_blk(ext3, node->i_block[13], (char*)indirect1) != 0)
                return -1;
        }
        if(ext3_read_blk(ext3, indirect1[num], (char*)indirect2) != 0)
            return -1;
        if(indirect2[pos] == 0) {
            indirect2[pos] = ext3_balloc(ext3);
            if(indirect2[pos] == 0)
                return -1;
            node->i_blocks += sectors_per_block;
            if(ext3_write_meta_blk(ext3, indirect1[num], (char*)indirect2) != 0)
                return -1;
            if(is_new != NULL)
                *is_new = 1;
        }
        *blk = (int32_t)indirect2[pos];
        return 0;
    }

    if(lbk < (int32_t)(entries_per_block * entries_per_block * entries_per_block +
            entries_per_block * entries_per_block + entries_per_block + 12)) {
        int32_t count = lbk - 12 - (int32_t)entries_per_block -
            (int32_t)(entries_per_block * entries_per_block);
        int32_t num1 = count / (int32_t)(entries_per_block * entries_per_block);
        int32_t rem = count % (int32_t)(entries_per_block * entries_per_block);
        int32_t num2 = rem / (int32_t)entries_per_block;
        int32_t pos = rem % (int32_t)entries_per_block;

        if(node->i_block[14] == 0) {
            node->i_block[14] = ext3_balloc(ext3);
            if(node->i_block[14] == 0)
                return -1;
            node->i_blocks += sectors_per_block;
            memset(indirect1, 0, block_size);
            if(ext3_write_meta_blk(ext3, node->i_block[14], (char*)indirect1) != 0)
                return -1;
        }
        if(ext3_read_blk(ext3, node->i_block[14], (char*)indirect1) != 0)
            return -1;
        if(indirect1[num1] == 0) {
            indirect1[num1] = ext3_balloc(ext3);
            if(indirect1[num1] == 0)
                return -1;
            node->i_blocks += sectors_per_block;
            memset(indirect2, 0, block_size);
            if(ext3_write_meta_blk(ext3, indirect1[num1], (char*)indirect2) != 0)
                return -1;
            if(ext3_write_meta_blk(ext3, node->i_block[14], (char*)indirect1) != 0)
                return -1;
        }
        if(ext3_read_blk(ext3, indirect1[num1], (char*)indirect2) != 0)
            return -1;
        if(indirect2[num2] == 0) {
            indirect2[num2] = ext3_balloc(ext3);
            if(indirect2[num2] == 0)
                return -1;
            node->i_blocks += sectors_per_block;
            memset(indirect3, 0, block_size);
            if(ext3_write_meta_blk(ext3, indirect2[num2], (char*)indirect3) != 0)
                return -1;
            if(ext3_write_meta_blk(ext3, indirect1[num1], (char*)indirect2) != 0)
                return -1;
        }
        if(ext3_read_blk(ext3, indirect2[num2], (char*)indirect3) != 0)
            return -1;
        if(indirect3[pos] == 0) {
            indirect3[pos] = ext3_balloc(ext3);
            if(indirect3[pos] == 0)
                return -1;
            node->i_blocks += sectors_per_block;
            if(ext3_write_meta_blk(ext3, indirect2[num2], (char*)indirect3) != 0)
                return -1;
            if(is_new != NULL)
                *is_new = 1;
        }
        *blk = (int32_t)indirect3[pos];
        return 0;
    }

    return -1;
}

int32_t ext3_write(ext3_t* ext3, EXT3_INODE* node, const char *data, int32_t nbytes, int32_t offset) {
    static char buf[EXT3_MAX_BLOCK_SIZE];
    const char *cq = data;
    char *cp;
    uint32_t now = ext3_now();
    uint32_t block_size = ext3_block_size(ext3);
    int32_t blk =0, lbk = 0, start_byte = 0, remain = 0;
    int32_t nbytes_copy = 0;
    while(nbytes > 0) {
        lbk = offset / (int32_t)block_size;
        start_byte = offset % (int32_t)block_size;

        if(start_byte == 0 && nbytes >= (int32_t)block_size) {
            int32_t max_blocks = nbytes / (int32_t)block_size;
            int32_t run_blocks = 0;
            int32_t first_blk = 0;

            while(run_blocks < max_blocks) {
                int32_t cur_blk = 0;
                if(ext3_ensure_data_block(ext3, node, lbk + run_blocks, &cur_blk, NULL) != 0)
                    break;
                if(run_blocks == 0)
                    first_blk = cur_blk;
                else if(cur_blk != (first_blk + run_blocks))
                    break;
                run_blocks++;
            }

            if(run_blocks > 0 && ext3_write_blocks_io(ext3, first_blk, cq, (uint32_t)run_blocks) == 0) {
                int32_t wrote = run_blocks * (int32_t)block_size;
                nbytes_copy += wrote;
                nbytes -= wrote;
                offset += wrote;
                cq += wrote;
                if(offset > (int32_t)node->i_size)
                    node->i_size = offset;
                continue;
            }

            if(ext3_ensure_data_block(ext3, node, lbk, &blk, NULL) != 0)
                return nbytes_copy;
            if(ext3_write_data_blk(ext3, (uint32_t)blk, cq) != 0)
                return nbytes_copy;
            nbytes_copy += (int32_t)block_size;
            nbytes -= (int32_t)block_size;
            offset += (int32_t)block_size;
            cq += block_size;
            if(offset > (int32_t)node->i_size)
                node->i_size = offset;
        }
        else {
            int32_t fresh = 0;
            if(ext3_ensure_data_block(ext3, node, lbk, &blk, &fresh) != 0)
                return nbytes_copy;
            /* A freshly allocated block may still hold stale data from a
             * previously deleted file (freed blocks are no longer zeroed
             * on disk): start from zeroes instead of reading it back. */
            if(fresh != 0)
                memset(buf, 0, block_size);
            else if(ext3_read_blk(ext3, (uint32_t)blk, buf) != 0)
                return nbytes_copy;
            cp = buf + start_byte;
            remain = (int32_t)block_size - start_byte;

            int32_t min = 0;
            if(nbytes <= remain) {
                min = nbytes;
            }
            else {
                min = remain;
            }
            memcpy(cp, cq, min);
            nbytes_copy += min;
            nbytes -= min;
            remain -= min;
            offset += min;
            cq += min;
            if(offset > (int32_t)node->i_size) {
                node->i_size = offset;
            }

            if(ext3_write_data_blk(ext3, (uint32_t)blk, buf) != 0)
                return nbytes_copy;
        }
    }
    if(nbytes_copy > 0) {
        node->i_mtime = now;
        node->i_ctime = now;
    }
    return nbytes_copy;
}

static uint32_t ext3_search(ext3_t* ext3, EXT3_INODE *ip, const char *name, EXT3_DIR_T* dirp) {
    char *cp;
    EXT3_DIR_T  *dp;
    uint32_t block_size = ext3_block_size(ext3);
    uint32_t blocks = ext3_dir_block_count(ext3, ip);
    char buf[EXT3_MAX_BLOCK_SIZE];
    if(dirp != NULL)
        memset(dirp, 0, sizeof(EXT3_DIR_T));

    for(uint32_t lbk = 0; lbk < blocks; lbk++) {
        if(ext3_read_inode_block(ext3, ip, lbk, buf) != 0)
            return 0;
        dp = (EXT3_DIR_T *)buf;
        cp = buf;
        while (cp < (buf + block_size)){
            if(dp->name_len == 0 || dp->rec_len < 12 || dp->rec_len < need_len(dp->name_len) ||
                    (cp + dp->rec_len) > (buf + block_size))
                break;
            if(dp->inode != 0 && ext3_dirent_name_equals(dp, name)) {
                if(dirp != NULL) {
                    memcpy(dirp, dp, sizeof(EXT3_DIR_T));
                    if(dp->name_len < sizeof(dirp->name))
                        dirp->name[dp->name_len] = 0;
                }
                return dp->inode;
            }
            cp += dp->rec_len;
            dp = (EXT3_DIR_T *)cp;
        }
    }
    return 0;
}

#define MAX_DIR_DEPTH 32
static int32_t ext3_split_fname(const char* filename, str_t* name[]) {
    int32_t u, depth;
    depth = 0;

    char hold[SHORT_NAME_MAX];
    while(1) {
        u = 0;
        if(*filename == '/') filename++; //skip '/'

        while(*filename != '/') {
            hold[u] = *filename;
            u++;
            filename++;
            if(*filename == 0 || u >= (SHORT_NAME_MAX-1))
                break;
        }
        hold[u] = 0;
        name[depth] = str_new(hold);
        depth++;
        if(depth >= MAX_DIR_DEPTH)
            break;
        if(*filename != 0)
            filename++;
        else
            break;
    }
    return depth;
}

uint32_t ext3_ino_by_fname(ext3_t* ext3, const char* filename, EXT3_DIR_T* dirp) {
    char buf[EXT3_MAX_BLOCK_SIZE];
    uint32_t depth, i, ino;
    str_t* name[MAX_DIR_DEPTH];
    EXT3_INODE *ip;

    if(strcmp(filename, "/") == 0) {
        if(dirp != NULL) {
            memset(dirp, 0, sizeof(EXT3_DIR_T));
            dirp->inode = 2;
            dirp->file_type = EXT3_FT_DIR;
            dirp->name_len = 1;
            dirp->name[0] = '/';
            dirp->name[1] = 0;
        }
        return 2; //ino 2 for root;
    }
    depth = ext3_split_fname(filename, name);

    ino = 2; // ext2/ext3 root inode
    ip = get_node_by_ino(ext3, ino, buf);
    if(ip != NULL) {
        for (i=0; i<depth; i++) {
            ino = ext3_search(ext3, ip, CS(name[i]), dirp);
            if (ino == 0) {
                break;
            }
            ip = get_node_by_ino(ext3, ino, buf);
            if(ip == NULL) {
                ino = 0;
                break;
            }
        }
    }
    for (i=0; i<depth; i++) {
        str_free(name[i]);
    }
    return ino;
}

int32_t ext3_node_by_fname(ext3_t* ext3, const char* filename, EXT3_INODE* inode) {
    uint32_t ino = ext3_ino_by_fname(ext3, filename, NULL);
    if(ino == 0)
        return -1;
    return ext3_node_by_ino(ext3, ino, inode);
}

int32_t ext3_truncate(ext3_t* ext3, uint32_t ino, EXT3_INODE* node) {
    uint32_t now = ext3_now();

    if(ext3_free_inode_data(ext3, node) != 0)
        return -1;
    node->i_mtime = now;
    node->i_ctime = now;
    node->i_dtime = 0;
    if(ino != 0 && ext3_put_node(ext3, ino, node) != 0)
        return -1;
    return 0;
}

static int32_t ext3_remove_path(ext3_t* ext3, const char* fname, int32_t want_dir) {
    char buf[EXT3_MAX_BLOCK_SIZE];
    char inode_buf[EXT3_MAX_BLOCK_SIZE];
    char dir[FS_FULL_NAME_MAX];
    char name[FS_FULL_NAME_MAX];
    EXT3_DIR_T dirp;
    uint32_t now = ext3_now();
    int32_t is_dir;

    vfs_dir_name(fname, dir, FS_FULL_NAME_MAX);
    vfs_file_name(fname, name, FS_FULL_NAME_MAX);
    if(name[0] == 0 || strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        return -1;

    uint32_t fino = ext3_ino_by_fname(ext3, dir, NULL);
    if(fino == 0)
        return -1;

    EXT3_INODE* fnode = get_node_by_ino(ext3, fino, buf);
    if(fnode == NULL)
        return -1;

    uint32_t ino = ext3_search(ext3, fnode, name, &dirp);
    if(ino == 0)
        return -1;

    EXT3_INODE* node = get_node_by_ino(ext3, ino, inode_buf);
    if(node == NULL)
        return -1;

    is_dir = ((node->i_mode & 0xF000) == EXT3_S_IFDIR) || dirp.file_type == EXT3_FT_DIR;
    if(want_dir) {
        if(!is_dir || ext3_dir_is_empty(ext3, node) != 1)
            return -1;
    }
    else if(is_dir) {
        return -1;
    }

    if(ext3_rm_child(ext3, fnode, name) != 0)
        return -1;
    if(want_dir && fnode->i_links_count > 0)
        fnode->i_links_count--;
    fnode->i_mtime = now;
    fnode->i_ctime = now;
    if(ext3_put_node(ext3, fino, fnode) != 0)
        return -1;

    if(!want_dir && node->i_links_count > 1) {
        node->i_links_count--;
        node->i_ctime = now;
        node->i_dtime = 0;
        return ext3_put_node(ext3, ino, node);
    }

    if(ext3_free_inode_data(ext3, node) != 0)
        return -1;
    node->i_links_count = 0;
    node->i_dtime = now;
    node->i_ctime = now;
    node->i_mtime = now;
    if(ext3_put_node(ext3, ino, node) != 0)
        return -1;
    if(ext3_idealloc(ext3, ino) != 0)
        return -1;
    if(want_dir) { //keep bg_used_dirs_count in sync (see ext3_create_dir)
        uint32_t gidx = get_gd_index_by_ino(ext3, ino);
        if(ext3->gds[gidx].bg_used_dirs_count > 0)
            ext3->gds[gidx].bg_used_dirs_count--;
        mark_gd_dirty(ext3, gidx);
    }
    if(ext3_flush_meta(ext3) != 0)
        return -1;
    return 0;
}

int32_t ext3_unlink(ext3_t* ext3, const char* fname) {
    return ext3_remove_path(ext3, fname, 0);
}

int32_t ext3_node_by_ino(ext3_t* ext3, uint32_t ino, EXT3_INODE* node) {
    char buf[EXT3_MAX_BLOCK_SIZE];
    EXT3_INODE* p = get_node_by_ino(ext3, ino, buf);
    if(p == NULL)
        return -1;
    memcpy(node, p, sizeof(EXT3_INODE));
    return 0;
}

int32_t ext3_rmdir(ext3_t* ext3, const char *fname) {
    return ext3_remove_path(ext3, fname, 1);
}

static inline int32_t ext3_get_gd_num(ext3_t* ext3) {
    uint32_t count = ext3->super.s_blocks_count - ext3->super.s_first_data_block;
    int32_t ret = (int32_t)(count / ext3->super.s_blocks_per_group);
    if((count % ext3->super.s_blocks_per_group) != 0)
        ret++;
    return ret;
}

static int32_t ext3_get_gds(ext3_t* ext3) {
    int32_t gd_size = sizeof(EXT3_GD);
    uint32_t block_size = ext3_block_size(ext3);
    uint32_t gdt_block = ext3_gdt_start_block(ext3);
    ext3->group_num = ext3_get_gd_num(ext3);
    ext3->gds = (EXT3_GD*)malloc(gd_size * ext3->group_num);
    if(ext3->gds == NULL)
        return -1;

    int32_t gd_num = (int32_t)(block_size / gd_size);
    int32_t index = 0;
    while(1) {
        char buf[EXT3_MAX_BLOCK_SIZE];
        /* raw device read: the metadata cache is always empty where
         * this runs (mount time / post-recovery reload) */
        if(ext3->read_block((int32_t)gdt_block, buf) != 0)
            return -1;
        for(int32_t j = 0; j < gd_num; j++) {
            memcpy(&ext3->gds[index], buf + (j * gd_size), gd_size);
            index++;
            if(index >= ext3->group_num)
                return 0;
        }
        gdt_block++;
    }
    return 0;
}

/* ---------- journal plumbing ---------- */

/* translate a journal-file block index to its fs block number by
 * walking the journal inode's block tree.  Only ever reads (through
 * the cache-aware helpers), so it is safe to call while a transaction
 * is being written. */
static int32_t ext3_jbd_map(void* ctx, uint32_t jblock, uint32_t* fs_block) {
    ext3_t* ext3 = (ext3_t*)ctx;
    int32_t blk = 0;
    if(ext3_get_data_block(ext3, &ext3->journal_inode, (int32_t)jblock, &blk) != 0)
        return -1;
    if(blk == 0)
        return -1;
    *fs_block = (uint32_t)blk;
    return 0;
}

/* load the journal, replay it, then record the mount state.  On any
 * failure the journal is torn down again and the caller treats the
 * filesystem as unmountable (EXT3_ERR_JOURNAL). */
static int32_t ext3_setup_journal(ext3_t* ext3, uint32_t now) {
    uint32_t inum = ext3->super.s_journal_inum;
    char buf[EXT3_MAX_BLOCK_SIZE];

    /* only an internal journal (a file inside this fs) is supported:
     * external journal devices are rejected */
    if(inum == 0 || inum > ext3->super.s_inodes_count ||
            (ext3->super.s_feature_incompat & EXT3_FEATURE_INCOMPAT_JOURNAL_DEV) != 0) {
        return -1;
    }

    EXT3_INODE* ip = get_node_by_ino(ext3, inum, buf);
    if(ip == NULL) {
        return -1;
    }
    memcpy(&ext3->journal_inode, ip, sizeof(EXT3_INODE));

    jbd_t* j = (jbd_t*)malloc(sizeof(jbd_t));
    if(j == NULL)
        return -1;
    /* the journal file's own blocks are accessed with RAW device IO:
     * log blocks must never be served from (or written through) the
     * metadata cache */
    if(jbd_load(j, ext3->read_block, ext3->write_block, ext3->flush, ext3_jbd_map, ext3) != 0) {
        free(j);
        return -1;
    }
    if(j->block_size != ext3_block_size(ext3) ||
            (uint64_t)j->maxlen * j->block_size > (uint64_t)ext3->journal_inode.i_size) {
        jbd_free(j);
        free(j);
        return -1;
    }
    ext3->journal = j;
    ext3->journal_ino = inum;

    /* replay committed transactions found in the log */
    if(jbd_recover(j) != 0)
        goto failed;
    ext3->recovered_txns = j->recovered_txns;
    ext3->recovered_blocks = j->recovered_blocks;

    /* recovery may have restored a newer super block and group
     * descriptors to their home locations: the in-RAM copies read at
     * init predate the replay, reload both */
    {
        uint32_t block_size = ext3_block_size(ext3);
        int32_t super_blk = (block_size == EXT3_MIN_BLOCK_SIZE) ? 1 : 0;
        if(ext3->read_block(super_blk, buf) != 0)
            goto failed;
        memcpy(&ext3->super, buf + ((block_size == EXT3_MIN_BLOCK_SIZE) ? 0 : EXT3_MIN_BLOCK_SIZE),
                sizeof(EXT3_SUPER));
        free(ext3->gds);
        ext3->gds = NULL;
        if(ext3_get_gds(ext3) != 0)
            goto failed;
        memset(ext3->dirty_gds, 0, (size_t)ext3->group_num);
        ext3->next_alloc_block = ext3->super.s_first_data_block;
    }

    /* bound transactions well below the log size */
    uint32_t limit = (j->maxlen - j->first) / 4;
    if(limit < 16)
        limit = 16;
    if(limit > 1024)
        limit = 1024;
    ext3->journal_txn_limit = limit;

    /* record the mount: journal replayed, fs mounted (not clean yet).
     * ext3_commit makes it durable, so a crash right after this point
     * still finds a replayable (idempotent, now empty) journal. */
    ext3->super.s_mnt_count++;
    ext3->super.s_mtime = now;
    ext3->super.s_state &= (uint16_t)~EXT2_VALID_FS;
    ext3->super.s_feature_incompat |= EXT3_FEATURE_INCOMPAT_RECOVER;
    ext3->dirty_super = 1;
    if(ext3_commit(ext3) != 0)
        goto failed;
    return 0;

failed:
    jbd_free(ext3->journal);
    free(ext3->journal);
    ext3->journal = NULL;
    return -1;
}

/* write every cached (committed) block back to its home location and
 * mark the log empty.  On failure the cache is left intact: the next
 * ext3_commit() re-journals whatever is still cached, superseding the
 * old transaction, so nothing committed is ever orphaned. */
static int32_t ext3_checkpoint_cache(ext3_t* ext3) {
    if(ext3->journal == NULL)
        return 0;
    for(uint32_t i = 0; i < EXT3_CACHE_BUCKETS; i++) {
        ext3_blkcache_t* e = ext3->cache[i];
        while(e != NULL) {
            if(ext3->write_block((int32_t)e->blk, e->data) != 0)
                return -1;
            e = e->next;
        }
    }
    /* the checkpointed blocks must be durable BEFORE the journal
     * superblock declares the log empty */
    if(ext3->flush != NULL)
        ext3->flush();
    if(jbd_checkpoint_done(ext3->journal) != 0)
        return -1;
    ext3->commit_pending = 0;
    return 0;
}

int32_t ext3_commit(ext3_t* ext3) {
    if(ext3 == NULL)
        return -1;

    /* ext2 compatible mode: no journal, the deferred metadata writes
     * just go straight home (identical to the old ext2 library) */
    if(ext3->journal == NULL) {
        if(ext3_flush_meta(ext3) != 0) {
            ext3->error = 1;
            return -1;
        }
        return 0;
    }

    _in_commit = 1;
    if(ext3_flush_meta(ext3) != 0)
        goto failed;

    if(ext3->cache_size > 0 || ext3->journal->revoke_num > 0) {
        uint32_t n = ext3->cache_size;
        uint32_t* blocks = NULL;
        char** datas = NULL;

        if(n > 0) {
            blocks = (uint32_t*)malloc(n * sizeof(uint32_t));
            datas = (char**)malloc(n * sizeof(char*));
            if(blocks == NULL || datas == NULL) {
                free(blocks);
                free(datas);
                goto failed;
            }
        }

        /* blocks left cached by an earlier commit whose checkpoint
         * failed midway are re-journaled here: the new transaction
         * supersedes the old one, so publishing its journal
         * superblock cannot orphan anything. */
        uint32_t k = 0;
        for(uint32_t i = 0; i < EXT3_CACHE_BUCKETS && k < n; i++) {
            ext3_blkcache_t* e = ext3->cache[i];
            while(e != NULL && k < n) {
                blocks[k] = e->blk;
                datas[k] = e->data;
                k++;
                e = e->next;
            }
        }

        jbd_txn_t txn;
        memset(&txn, 0, sizeof(txn));
        txn.blocks = blocks;
        txn.datas = datas;
        txn.count = k;
        txn.revokes = ext3->journal->revoke_list;
        txn.revoke_count = ext3->journal->revoke_num;

        int32_t ret = jbd_commit(ext3->journal, &txn);
        free(blocks);
        free(datas);
        if(ret != 0)
            goto failed;

        ext3->commit_pending = 1;
        if(ext3_checkpoint_cache(ext3) != 0)
            goto failed;
        jbd_clear_revokes(ext3->journal);
        ext3_cache_free_all(ext3);
    }

    _in_commit = 0;
    return 0;

failed:
    _in_commit = 0;
    ext3->error = 1;
    return -1;
}

int32_t ext3_init(ext3_t* ext3, read_block_func_t read_block, write_block_func_t write_block, uint32_t buffer_size) {
    return ext3_init_ex2(ext3, read_block, NULL, write_block, NULL, NULL, buffer_size);
}

int32_t ext3_init_ex(ext3_t* ext3, read_block_func_t read_block, read_blocks_func_t read_blocks,
        write_block_func_t write_block, write_blocks_func_t write_blocks, uint32_t buffer_size) {
    return ext3_init_ex2(ext3, read_block, read_blocks, write_block, write_blocks, NULL, buffer_size);
}

int32_t ext3_init_ex2(ext3_t* ext3, read_block_func_t read_block, read_blocks_func_t read_blocks,
        write_block_func_t write_block, write_blocks_func_t write_blocks,
        flush_func_t flush, uint32_t buffer_size) {
    char buf[EXT3_MAX_BLOCK_SIZE];
    int32_t ret;

    if(ext3 == NULL || read_block == NULL || write_block == NULL)
        return EXT3_ERR_IO;

    memset(ext3, 0, sizeof(ext3_t));
    ext3->read_block = read_block;
    ext3->read_blocks = read_blocks;
    ext3->write_block = write_block;
    ext3->write_blocks = write_blocks;
    ext3->flush = flush;

    _cached_block_bitmap_blk = 0;
    _cached_block_bitmap_dirty = 0;
    _cached_inode_bitmap_blk = 0;
    _cached_inode_bitmap_dirty = 0;
    _cached_indirect_blk = 0;
    _cached_indirect_dirty = 0;
    memset(_cached_indirect_block, 0, sizeof(_cached_indirect_block));

    if(_cached_block_bitmap == NULL)
        _cached_block_bitmap = (char*)malloc(EXT3_MAX_BLOCK_SIZE);
    if(_cached_inode_bitmap == NULL)
        _cached_inode_bitmap = (char*)malloc(EXT3_MAX_BLOCK_SIZE);
    if(_cached_block_bitmap == NULL || _cached_inode_bitmap == NULL) {
        free(_cached_block_bitmap);
        free(_cached_inode_bitmap);
        _cached_block_bitmap = NULL;
        _cached_inode_bitmap = NULL;
        return EXT3_ERR_IO;
    }

    sd_set_block_size(EXT3_DEFAULT_BLOCK_SIZE);

    //read super block
    if(ext3->read_block(1, buf) != 0) {
        return EXT3_ERR_IO;
    }
    memcpy(&ext3->super, buf, sizeof(EXT3_SUPER));
    if(ext3_validate_super(ext3) != 0) {
        return EXT3_ERR_IO;
    }
    if(sd_set_block_size(ext3_block_size(ext3)) != 0) {
        return EXT3_ERR_IO;
    }
    if(ext3_get_gds(ext3) != 0) {
        return EXT3_ERR_IO;
    }
    ext3->dirty_gds = (uint8_t*)calloc((size_t)ext3->group_num, sizeof(uint8_t));
    if(ext3->dirty_gds == NULL) {
        ret = EXT3_ERR_IO;
        goto failed;
    }
    ext3->next_alloc_block = ext3->super.s_first_data_block;
    sd_set_max_sector_index(ext3->super.s_blocks_count * (ext3_block_size(ext3) / SECTOR_SIZE));
    sd_set_buffer_size(buffer_size);

    /* ext3 preferred: when the fs carries a journal, load it, replay
     * it and journal all further metadata updates */
    if((ext3->super.s_feature_compat & EXT3_FEATURE_COMPAT_HAS_JOURNAL) != 0) {
        int32_t jret = ext3_setup_journal(ext3, ext3_now());
        if(jret != 0) {
            ret = EXT3_ERR_JOURNAL;
            goto failed;
        }
    }
    return 0;

failed:
    _in_commit = 0;
    ext3_cache_free_all(ext3);
    free(_cached_block_bitmap);
    free(_cached_inode_bitmap);
    _cached_block_bitmap = NULL;
    _cached_inode_bitmap = NULL;
    free(ext3->dirty_gds);
    ext3->dirty_gds = NULL;
    free(ext3->gds);
    ext3->gds = NULL;
    return ret;
}

int32_t ext3_probe(read_block_func_t read_block) {
    char buf[EXT3_MAX_BLOCK_SIZE];
    EXT3_SUPER super;
    int32_t ret = EXT3_PROBE_NONE;

    if(read_block == NULL)
        return EXT3_PROBE_NONE;
    if(sd_set_block_size(EXT3_DEFAULT_BLOCK_SIZE) != 0)
        return EXT3_PROBE_NONE;
    if(read_block(1, buf) != 0)
        return EXT3_PROBE_NONE;
    memcpy(&super, buf, sizeof(EXT3_SUPER));

    if(super.s_magic == EXT3_SUPER_MAGIC) {
        /* ext3 first: a usable internal journal makes it ext3 */
        if((super.s_feature_compat & EXT3_FEATURE_COMPAT_HAS_JOURNAL) != 0 &&
                super.s_journal_inum != 0 &&
                (super.s_feature_incompat & EXT3_FEATURE_INCOMPAT_JOURNAL_DEV) == 0)
            ret = EXT3_PROBE_EXT3;
        else
            ret = EXT3_PROBE_EXT2;
        /* leave the device at the filesystem's block size */
        (void)sd_set_block_size(EXT3_MIN_BLOCK_SIZE << super.s_log_block_size);
    }
    return ret;
}

void ext3_quit(ext3_t* ext3) {
    if(ext3 == NULL)
        return;

    (void)ext3_commit(ext3);

    if(ext3->journal != NULL) {
        /* clean unmount: drop RECOVER and mark the fs valid (or
         * errored), then commit so the state is durable */
        ext3->super.s_wtime = ext3_now();
        ext3->super.s_feature_incompat &= ~(uint32_t)EXT3_FEATURE_INCOMPAT_RECOVER;
        if(ext3->error != 0)
            ext3->super.s_state = EXT2_ERROR_FS;
        else
            ext3->super.s_state = EXT2_VALID_FS;
        ext3->dirty_super = 1;
        (void)ext3_commit(ext3);
        jbd_free(ext3->journal);
        free(ext3->journal);
        ext3->journal = NULL;
    }

    free(_cached_block_bitmap);
    free(_cached_inode_bitmap);
    _cached_block_bitmap = NULL;
    _cached_inode_bitmap = NULL;
    _cached_block_bitmap_blk = 0;
    _cached_inode_bitmap_blk = 0;
    _cached_block_bitmap_dirty = 0;
    _cached_inode_bitmap_dirty = 0;
    _cached_indirect_blk = 0;
    _cached_indirect_dirty = 0;
    memset(_cached_indirect_block, 0, sizeof(_cached_indirect_block));
    free(ext3->dirty_gds);
    ext3->dirty_gds = NULL;
    free(ext3->gds);
    ext3->gds = NULL;
    ext3_cache_free_all(ext3);
}

void* ext3_readfile(ext3_t* ext3, const char* fname, int32_t* size) {
    void* ret = NULL;
    uint32_t block_size = ext3_block_size(ext3);
    if(size != NULL)
        *size = -1;

    uint32_t ino = ext3_ino_by_fname(ext3, fname, NULL);
    if(ino > 0) {
        EXT3_INODE inode;
        if(ext3_node_by_ino(ext3, ino, &inode) != 0) {
            return ret;
        }

        char *data = (char*)malloc(inode.i_size + 1);
        if(data != NULL) {
            ret = data;
            uint32_t rd = 0;
            while(rd < inode.i_size) {
                /* never ask for more than the buffer holds: the read
                 * chunk is bounded by the file's remaining size, not
                 * just the block size */
                uint32_t chunk = inode.i_size - rd;
                if(chunk > block_size)
                    chunk = block_size;
                int sz = ext3_read(ext3, &inode, data, (int32_t)chunk, (int32_t)rd);
                if(sz <= 0)
                    break;
                data += sz;
                rd += sz;
            }
            if(size != NULL)
                *size = (int32_t)rd;
        }
    }
    return ret;
}
