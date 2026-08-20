#include <sd/sd.h>
#include <ext2/ext2fs.h>
#include <ewoksys/vfs.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ewoksys/mstr.h>

#define SHORT_NAME_MAX 64

static int32_t ext2_bdealloc(ext2_t* ext2, uint32_t block);
static int32_t need_len(int32_t len);
static int32_t ext2_ensure_data_block(ext2_t* ext2, INODE* node, int32_t lbk, int32_t* blk, int32_t* is_new);
static int32_t ext2_flush_meta(ext2_t* ext2);
static char* ext2_get_block_bitmap(ext2_t* ext2, uint32_t blk);
static char* ext2_get_inode_bitmap(ext2_t* ext2, uint32_t blk);
static uint32_t* ext2_get_cached_indirect_block(ext2_t* ext2, uint32_t blk);

static uint32_t _cached_block_bitmap_blk = 0;
static uint8_t _cached_block_bitmap_dirty = 0;
static char* _cached_block_bitmap = NULL;
static uint32_t _cached_inode_bitmap_blk = 0;
static uint8_t _cached_inode_bitmap_dirty = 0;
static char* _cached_inode_bitmap = NULL;
static uint32_t _cached_indirect_blk = 0;
static uint8_t _cached_indirect_dirty = 0;
static uint32_t _cached_indirect_block[EXT2_MAX_BLOCK_SIZE / sizeof(uint32_t)];

static int32_t ext2_validate_super(ext2_t* ext2) {
    uint32_t unsupported_incompat = ext2->super.s_feature_incompat & ~EXT2_FEATURE_INCOMPAT_FILETYPE;
    uint32_t unsupported_ro = ext2->super.s_feature_ro_compat &
        ~(EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER | EXT2_FEATURE_RO_COMPAT_LARGE_FILE);

    if(ext2->super.s_magic != EXT2_SUPER_MAGIC)
        return -1;
    uint32_t block_size = ext2_block_size(ext2);
    if(block_size != 1024 && block_size != 2048 && block_size != 4096)
        return -1;
    if(ext2_inode_size(ext2) < sizeof(INODE))
        return -1;
    if(unsupported_incompat != 0 || unsupported_ro != 0)
        return -1;
    return 0;
}

static uint32_t ext2_now(void) {
    time_t now = time(NULL);
    return (now <= 0) ? 0U : (uint32_t)now;
}

static int32_t ext2_dirent_name_equals(const DIR_T* dp, const char* name) {
    size_t len = strlen(name);
    return dp->name_len == len && memcmp(dp->name, name, len) == 0;
}

static uint32_t ext2_dir_block_count(ext2_t* ext2, const INODE* node) {
    uint32_t block_size = ext2_block_size(ext2);
    if(node->i_size == 0)
        return 0;
    return (node->i_size + block_size - 1) / block_size;
}

static int32_t ext2_read_blocks_io(ext2_t* ext2, int32_t block, void* buf, uint32_t count) {
    char* p = (char*)buf;
    uint32_t block_size = ext2_block_size(ext2);

    if(count == 0)
        return 0;
    if(ext2->read_blocks != NULL)
        return ext2->read_blocks(block, buf, count);

    for(uint32_t i = 0; i < count; i++) {
        if(ext2->read_block(block + (int32_t)i, p + (i * block_size)) != 0)
            return -1;
    }
    return 0;
}

static int32_t ext2_write_blocks_io(ext2_t* ext2, int32_t block, const void* buf, uint32_t count) {
    const char* p = (const char*)buf;
    uint32_t block_size = ext2_block_size(ext2);

    if(count == 0)
        return 0;
    if(ext2->write_blocks != NULL)
        return ext2->write_blocks(block, buf, count);

    for(uint32_t i = 0; i < count; i++) {
        if(ext2->write_block(block + (int32_t)i, p + (i * block_size)) != 0)
            return -1;
    }
    return 0;
}

/*
 * Returns 0 with *blk == 0 for a sparse hole (a never-allocated block,
 * e.g. created by lseek past EOF + write); -1 only for real I/O errors
 * or an out-of-range logical block. Callers must check *blk == 0.
 */
static int32_t ext2_get_data_block(ext2_t* ext2, INODE* node, int32_t lbk, int32_t* blk) {
    uint32_t entries_per_block = ext2_indirect_entries(ext2);
    uint32_t ptr_buf[EXT2_MAX_BLOCK_SIZE / sizeof(uint32_t)];

    *blk = 0;
    if(lbk < 0)
        return -1;
    if(lbk < 12) {
        *blk = node->i_block[lbk];
        return 0;
    }
    else if(lbk < (int32_t)(entries_per_block + 12)) {
        uint32_t* ptr_buf;
        if(node->i_block[12] == 0)
            return 0;
        ptr_buf = ext2_get_cached_indirect_block(ext2, node->i_block[12]);
        if(ptr_buf == NULL)
            return -1;
        *blk = (int32_t)ptr_buf[lbk - 12];
        return 0;
    }
    else if(lbk < (int32_t)(entries_per_block * entries_per_block + entries_per_block + 12)) {
        int32_t count = lbk - 12 - (int32_t)entries_per_block;
        int32_t num = count / (int32_t)entries_per_block;
        int32_t pos_offset = count % (int32_t)entries_per_block;
        uint32_t indirect1[EXT2_MAX_BLOCK_SIZE / sizeof(uint32_t)];
        if(node->i_block[13] == 0)
            return 0;
        if(ext2->read_block(node->i_block[13], (char*)indirect1) != 0)
            return -1;
        if(indirect1[num] == 0)
            return 0;
        if(ext2->read_block((int32_t)indirect1[num], (char*)ptr_buf) != 0)
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
        uint32_t indirect1[EXT2_MAX_BLOCK_SIZE / sizeof(uint32_t)];
        uint32_t indirect2[EXT2_MAX_BLOCK_SIZE / sizeof(uint32_t)];
        if(node->i_block[14] == 0)
            return 0;
        if(ext2->read_block(node->i_block[14], (char*)indirect1) != 0)
            return -1;
        if(indirect1[num1] == 0)
            return 0;
        if(ext2->read_block((int32_t)indirect1[num1], (char*)indirect2) != 0)
            return -1;
        if(indirect2[num2] == 0)
            return 0;
        if(ext2->read_block((int32_t)indirect2[num2], (char*)ptr_buf) != 0)
            return -1;
        *blk = (int32_t)ptr_buf[pos_offset];
        return 0;
    }
    return -1;
}

static int32_t ext2_read_inode_block(ext2_t* ext2, INODE* node, uint32_t lbk, char* buf) {
    int32_t blk = 0;
    if(ext2_get_data_block(ext2, node, (int32_t)lbk, &blk) != 0 || blk == 0)
        return -1;
    return ext2->read_block(blk, buf);
}

/* Read an indirect-pointer block, preferring the dirty cached copy:
 * ext2_ensure_data_block defers the write-back of a single-indirect
 * block, so a direct ext2->read_block on it would return stale card
 * data (wrong i_blocks accounting, missed blocks on truncate/unlink). */
static int32_t ext2_read_indirect_block(ext2_t* ext2, uint32_t blk, uint32_t* buf) {
    if(blk == 0)
        return -1;
    if(_cached_indirect_blk == blk) {
        memcpy(buf, _cached_indirect_block, ext2_block_size(ext2));
        return 0;
    }
    return ext2->read_block((int32_t)blk, (char*)buf);
}

static uint32_t ext2_count_indirect_blocks(ext2_t* ext2, uint32_t blk, int32_t depth) {
    uint32_t block_size = ext2_block_size(ext2);
    uint32_t ptr_buf[EXT2_MAX_BLOCK_SIZE / sizeof(uint32_t)];
    uint32_t count = 1;
    uint32_t entries_per_block = block_size / sizeof(uint32_t);

    if(blk == 0)
        return 0;
    if(ext2_read_indirect_block(ext2, blk, ptr_buf) != 0)
        return count;

    for(uint32_t i = 0; i < entries_per_block; i++) {
        if(ptr_buf[i] == 0)
            continue;
        if(depth == 1)
            count++;
        else
            count += ext2_count_indirect_blocks(ext2, ptr_buf[i], depth - 1);
    }
    return count;
}

static uint32_t ext2_inode_reserved_sectors(ext2_t* ext2, const INODE* node) {
    uint32_t blocks = 0;
    uint32_t sectors_per_block = ext2_block_size(ext2) / SECTOR_SIZE;

    for(int32_t i = 0; i < 12; i++) {
        if(node->i_block[i] != 0)
            blocks++;
    }
    blocks += ext2_count_indirect_blocks(ext2, node->i_block[12], 1);
    blocks += ext2_count_indirect_blocks(ext2, node->i_block[13], 2);
    blocks += ext2_count_indirect_blocks(ext2, node->i_block[14], 3);
    return blocks * sectors_per_block;
}

static int32_t ext2_free_indirect_tree(ext2_t* ext2, uint32_t blk, int32_t depth) {
    uint32_t block_size = ext2_block_size(ext2);
    uint32_t ptr_buf[EXT2_MAX_BLOCK_SIZE / sizeof(uint32_t)];
    uint32_t entries_per_block = block_size / sizeof(uint32_t);

    if(blk == 0)
        return 0;
    if(ext2_read_indirect_block(ext2, blk, ptr_buf) != 0)
        return -1;

    for(uint32_t i = 0; i < entries_per_block; i++) {
        if(ptr_buf[i] == 0)
            continue;
        if(depth == 1) {
            if(ext2_bdealloc(ext2, ptr_buf[i]) != 0)
                return -1;
        }
        else if(ext2_free_indirect_tree(ext2, ptr_buf[i], depth - 1) != 0) {
            return -1;
        }
    }
    return ext2_bdealloc(ext2, blk);
}

static int32_t ext2_free_inode_data(ext2_t* ext2, INODE* node) {
    for(int32_t i = 0; i < 12; i++) {
        if(node->i_block[i] == 0)
            continue;
        if(ext2_bdealloc(ext2, node->i_block[i]) != 0)
            return -1;
        node->i_block[i] = 0;
    }
    if(ext2_free_indirect_tree(ext2, node->i_block[12], 1) != 0)
        return -1;
    if(ext2_free_indirect_tree(ext2, node->i_block[13], 2) != 0)
        return -1;
    if(ext2_free_indirect_tree(ext2, node->i_block[14], 3) != 0)
        return -1;
    node->i_block[12] = 0;
    node->i_block[13] = 0;
    node->i_block[14] = 0;
    node->i_size = 0;
    node->i_blocks = 0;
    return 0;
}

static int32_t ext2_dir_is_empty(ext2_t* ext2, INODE* node) {
    uint32_t block_size = ext2_block_size(ext2);
    uint32_t blocks = ext2_dir_block_count(ext2, node);
    char buf[EXT2_MAX_BLOCK_SIZE];

    for(uint32_t lbk = 0; lbk < blocks; lbk++) {
        char* cp = buf;
        if(ext2_read_inode_block(ext2, node, lbk, buf) != 0)
            return -1;
        while(cp < (buf + block_size)) {
            DIR_T* dp = (DIR_T*)cp;
            if(dp->name_len == 0 || dp->rec_len < 12 || dp->rec_len < need_len(dp->name_len) ||
                    (cp + dp->rec_len) > (buf + block_size)) {
                break;
            }
            if(dp->inode != 0 &&
                    !ext2_dirent_name_equals(dp, ".") &&
                    !ext2_dirent_name_equals(dp, "..")) {
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
static inline uint32_t get_gd_index_by_ino(ext2_t* ext2, uint32_t ino) {
    return ((ino -1) / ext2->super.s_inodes_per_group);
}

/*get group descriptor index by block*/
static inline uint32_t get_gd_index_by_block(ext2_t* ext2, uint32_t block) {
    return ((block - ext2->super.s_first_data_block) / ext2->super.s_blocks_per_group);
}

/*get inode index in group*/
static inline uint32_t get_ino_in_group(ext2_t* ext2, uint32_t ino, uint32_t index) {
    return ino - (index * ext2->super.s_inodes_per_group);
}

/*get block index in group*/
static inline uint32_t get_block_in_group(ext2_t* ext2, uint32_t block, uint32_t index) {
    return block - ext2_group_start_block(ext2, index);
}

/*write back group descriptor by index*/
static int32_t set_gd(ext2_t* ext2, uint32_t index) {
    uint32_t gd_size = sizeof(GD);
    uint32_t block_size = ext2_block_size(ext2);
    uint32_t gd_num = (block_size / gd_size);
    char buf[EXT2_MAX_BLOCK_SIZE];

    uint32_t blk_index = (index / gd_num);
    uint32_t first = blk_index * gd_num;
    uint32_t count = ext2->group_num - first;

    if(count > gd_num)
        count = gd_num;
    memset(buf, 0, block_size);
    memcpy(buf, &ext2->gds[first], count * gd_size);
    return ext2->write_block((int32_t)(ext2_gdt_start_block(ext2) + blk_index), buf);
}

/*write back super block*/
static int32_t set_super(ext2_t* ext2) {
    uint32_t block_size = ext2_block_size(ext2);
    char buf[EXT2_MAX_BLOCK_SIZE];
    int32_t super_block = (block_size == EXT2_MIN_BLOCK_SIZE) ? 1 : 0;

    if(ext2->read_block(super_block, buf) != 0)
        return -1;
    if(block_size == EXT2_MIN_BLOCK_SIZE)
        memcpy(buf, &ext2->super, sizeof(SUPER));
    else
        memcpy(buf + EXT2_MIN_BLOCK_SIZE, &ext2->super, sizeof(SUPER));
    return ext2->write_block(super_block, buf);
}

static void mark_gd_dirty(ext2_t* ext2, uint32_t index) {
        if(ext2->dirty_gds != NULL && index < (uint32_t)ext2->group_num)
                ext2->dirty_gds[index] = 1;
}

static int32_t ext2_flush_meta(ext2_t* ext2) {
        if(_cached_indirect_blk != 0 && _cached_indirect_dirty != 0) {
                if(ext2->write_block((int32_t)_cached_indirect_blk, (char*)_cached_indirect_block) != 0)
                        return -1;
                _cached_indirect_dirty = 0;
        }

        if(_cached_block_bitmap != NULL &&
                        _cached_block_bitmap_blk != 0 &&
                        _cached_block_bitmap_dirty != 0) {
                if(ext2->write_block((int32_t)_cached_block_bitmap_blk, _cached_block_bitmap) != 0)
                        return -1;
                _cached_block_bitmap_dirty = 0;
        }

        if(_cached_inode_bitmap != NULL &&
                        _cached_inode_bitmap_blk != 0 &&
                        _cached_inode_bitmap_dirty != 0) {
                if(ext2->write_block((int32_t)_cached_inode_bitmap_blk, _cached_inode_bitmap) != 0)
                        return -1;
                _cached_inode_bitmap_dirty = 0;
        }

        if(ext2->dirty_gds != NULL) {
                for(uint32_t i = 0; i < (uint32_t)ext2->group_num; i++) {
                        if(ext2->dirty_gds[i] != 0) {
                                if(set_gd(ext2, i) != 0)
                                        return -1;
                                ext2->dirty_gds[i] = 0;
                        }
                }
        }

        if(ext2->dirty_super != 0) {
                if(set_super(ext2) != 0)
                        return -1;
                ext2->dirty_super = 0;
        }
        return 0;
}

static char* ext2_get_block_bitmap(ext2_t* ext2, uint32_t blk) {
        if(_cached_block_bitmap == NULL)
                return NULL;
        if(_cached_block_bitmap_blk != blk) {
                if(_cached_block_bitmap_blk != 0 && _cached_block_bitmap_dirty != 0) {
                        if(ext2->write_block((int32_t)_cached_block_bitmap_blk, _cached_block_bitmap) != 0)
                                return NULL;
                        _cached_block_bitmap_dirty = 0;
                }
                if(ext2->read_block((int32_t)blk, _cached_block_bitmap) != 0)
                        return NULL;
                _cached_block_bitmap_blk = blk;
        }
        return _cached_block_bitmap;
}

static char* ext2_get_inode_bitmap(ext2_t* ext2, uint32_t blk) {
        if(_cached_inode_bitmap == NULL)
                return NULL;
        if(_cached_inode_bitmap_blk != blk) {
                if(_cached_inode_bitmap_blk != 0 && _cached_inode_bitmap_dirty != 0) {
                        if(ext2->write_block((int32_t)_cached_inode_bitmap_blk, _cached_inode_bitmap) != 0)
                                return NULL;
                        _cached_inode_bitmap_dirty = 0;
                }
                if(ext2->read_block((int32_t)blk, _cached_inode_bitmap) != 0)
                        return NULL;
                _cached_inode_bitmap_blk = blk;
        }
        return _cached_inode_bitmap;
}

static uint32_t* ext2_get_cached_indirect_block(ext2_t* ext2, uint32_t blk) {
        if(blk == 0)
                return NULL;

        if(_cached_indirect_blk != blk) {
                if(_cached_indirect_blk != 0 && _cached_indirect_dirty != 0) {
                        if(ext2->write_block((int32_t)_cached_indirect_blk, (char*)_cached_indirect_block) != 0)
                                return NULL;
                        _cached_indirect_dirty = 0;
                }
                if(ext2->read_block((int32_t)blk, (char*)_cached_indirect_block) != 0)
                        return NULL;
                _cached_indirect_blk = blk;
        }
        return _cached_indirect_block;
}

static void inc_free_blocks(ext2_t* ext2, uint32_t block) {
    int32_t index = get_gd_index_by_block(ext2, block);
    ext2->gds[index].bg_free_blocks_count++;
        mark_gd_dirty(ext2, (uint32_t)index);

    ext2->super.s_free_blocks_count++;
        ext2->dirty_super = 1;
}

static void inc_free_inodes(ext2_t* ext2, uint32_t ino) {
    uint32_t index = get_gd_index_by_ino(ext2, ino);
    ext2->gds[index].bg_free_inodes_count++;
        mark_gd_dirty(ext2, index);

    ext2->super.s_free_inodes_count++;
        ext2->dirty_super = 1;
}

static void dec_free_blocks(ext2_t* ext2, uint32_t block) {
    uint32_t index = get_gd_index_by_block(ext2, block);
    ext2->gds[index].bg_free_blocks_count--;
        mark_gd_dirty(ext2, index);

    ext2->super.s_free_blocks_count--;
        ext2->dirty_super = 1;
}

static void dec_free_inodes(ext2_t* ext2, uint32_t ino) {
    uint32_t index = get_gd_index_by_ino(ext2, ino);
    ext2->gds[index].bg_free_inodes_count--;
        mark_gd_dirty(ext2, index);

    ext2->super.s_free_inodes_count--;
        ext2->dirty_super = 1;
}

static int32_t ext2_idealloc(ext2_t* ext2, uint32_t ino) {
    if (ino > ext2->super.s_inodes_count)
        return -1;

    // get inode bitmap block
    uint32_t index = get_gd_index_by_ino(ext2, ino);
    uint32_t ino_g = get_ino_in_group(ext2, ino, index);

    //uint32_t blk = index*ext2->super.s_blocks_per_group + ext2->gds[index].bg_inode_bitmap;
    uint32_t blk = ext2->gds[index].bg_inode_bitmap;
        char* buf = ext2_get_inode_bitmap(ext2, blk);
        if(buf == NULL)
        return -1;

    clr_bit(buf, (int32_t)(ino_g - 1));
        _cached_inode_bitmap_dirty = 1;
    inc_free_inodes(ext2, ino);
    return 0;
}

static int32_t ext2_bdealloc(ext2_t* ext2, uint32_t block) {
    if (block == 0)
        return -1;

        if(_cached_indirect_blk == block) {
                _cached_indirect_blk = 0;
                _cached_indirect_dirty = 0;
                memset(_cached_indirect_block, 0, sizeof(_cached_indirect_block));
        }

    uint32_t index = get_gd_index_by_block(ext2, block);
    uint32_t block_g = get_block_in_group(ext2, block, index);

    //uint32_t blk = index * ext2->super.s_blocks_per_group + ext2->gds[index].bg_block_bitmap;
    uint32_t blk = ext2->gds[index].bg_block_bitmap;
        char* buf = ext2_get_block_bitmap(ext2, blk);
        if(buf == NULL)
        return -1;

    clr_bit(buf, (int32_t)block_g);
        _cached_block_bitmap_dirty = 1;
    // update free inode count in SUPER and GD
    inc_free_blocks(ext2, block);

    /* Do NOT zero the freed block on disk: unlinking/truncating a large
     * file would issue one synchronous SD write (plus a read-back verify
     * on real hosts) per block, which pushed rm of a multi-MB file past
     * the kernel's 10s IPC timeout and got the unlink aborted mid-flight.
     * Freed blocks may now hold stale data; every path that reads a
     * freshly allocated block before writing it must not assume zeroes
     * (ext2_write memsets partial-write targets, dir/indirect blocks are
     * always initialized in memory before their first write). */
    return 0;
}

static uint32_t ext2_ialloc(ext2_t* ext2) {  //alloc a node, inode start with 1 not 0!!
    uint32_t index = 0;
    uint32_t i, blk = 0, ino = 0;
    for (i=0; i < ext2->super.s_inodes_count; i++){
        ino = i + 1;
        if((i % ext2->super.s_inodes_per_group) == 0) {
            index = get_gd_index_by_ino(ext2, ino);
            //blk = index*ext2->super.s_inodes_per_group + ext2->gds[index].bg_inode_bitmap;
            blk = ext2->gds[index].bg_inode_bitmap;
                        if(ext2_get_inode_bitmap(ext2, blk) == NULL)
                return 0;
        }
    
        uint32_t ino_g = get_ino_in_group(ext2, ino, index);
                        char* buf = _cached_inode_bitmap;
        if (tst_bit(buf, ino_g-1) == 0){
            set_bit(buf, (int32_t)(ino_g - 1));
                        _cached_inode_bitmap_dirty = 1;
            // update free inode count in SUPER and GD
            dec_free_inodes(ext2, ino);
            return ino;
        }
    }
    return 0;
} 

static int32_t ext2_balloc(ext2_t* ext2) { //alloc a block
    uint32_t index = 0;
    uint32_t blk = 0;

    uint32_t start = ext2->next_alloc_block;
    if(start < ext2->super.s_first_data_block || start >= ext2->super.s_blocks_count)
        start = ext2->super.s_first_data_block;

    for(uint32_t pass = 0; pass < 2; pass++) {
        uint32_t begin = (pass == 0) ? start : ext2->super.s_first_data_block;
        uint32_t end = (pass == 0) ? ext2->super.s_blocks_count : start;

        for(uint32_t block = begin; block < end; block++) {
            if(((block - ext2->super.s_first_data_block) % ext2->super.s_blocks_per_group) == 0) {
                index = get_gd_index_by_block(ext2, block);
                blk = ext2->gds[index].bg_block_bitmap;
                                if(ext2_get_block_bitmap(ext2, blk) == NULL)
                    return 0;
            }

            uint32_t block_g = get_block_in_group(ext2, block, index);
                        char* buf = _cached_block_bitmap;
            if(tst_bit(buf, (int32_t)block_g) == 0) {
                set_bit(buf, (int32_t)block_g);
                                _cached_block_bitmap_dirty = 1;
                dec_free_blocks(ext2, block);
                ext2->next_alloc_block = block + 1;
                if(ext2->next_alloc_block >= ext2->super.s_blocks_count)
                    ext2->next_alloc_block = ext2->super.s_first_data_block;
                return block;
            }
        }
    }
    return 0;
}

static int32_t need_len(int32_t len) {
    return 4 * ((8 + len + 3) / 4);
}

static int32_t write_child(ext2_t* ext2, INODE* pip, uint32_t ino, const char *name, int32_t ftype) {
    int32_t nlen, ideal_len, remain, blk;
    uint32_t now = ext2_now();
    uint32_t block_size = ext2_block_size(ext2);
    uint32_t name_len = strlen(name);
    uint32_t blocks = ext2_dir_block_count(ext2, pip);
    char buf[EXT2_MAX_BLOCK_SIZE];
    char *cp;
    DIR_T *dp = NULL;
    //(1)-(3)
    nlen = need_len((int32_t)name_len);
    for(uint32_t lbk = 0; ; lbk++) {
        if(lbk >= blocks) {
            if(ext2_ensure_data_block(ext2, pip, (int32_t)lbk, &blk, NULL) != 0)
                return -1;
            memset(buf, 0, block_size);
            dp = (DIR_T *)buf;
            dp->inode = ino;
            dp->rec_len = (uint16_t)block_size;
            dp->name_len = (uint8_t)name_len;
            dp->file_type = (uint8_t)ftype;
            strcpy(dp->name, name);
            pip->i_size = (lbk + 1) * block_size;
            pip->i_blocks = ext2_inode_reserved_sectors(ext2, pip);
            pip->i_mtime = now;
            pip->i_ctime = now;
            return ext2->write_block(blk, buf);
        }

        if(ext2_get_data_block(ext2, pip, (int32_t)lbk, &blk) != 0 || blk == 0)
            return -1;
        if(ext2->read_block(blk, buf) != 0)
            return -1;
        cp = buf;
        dp = (DIR_T *)cp;
        if(dp->inode == 0) {
            dp->inode = ino;
            dp->rec_len = (uint16_t)block_size;
            dp->name_len = (uint8_t)name_len;
            dp->file_type = (uint8_t)ftype;
            strcpy(dp->name, name);
            pip->i_mtime = now;
            pip->i_ctime = now;
            return ext2->write_block(blk, buf);
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
            dp = (DIR_T *)cp;
        }
        if(dp->rec_len == 0 || dp->rec_len < need_len(dp->name_len) || (cp + dp->rec_len) > (buf + block_size))
            dp->rec_len = (uint16_t)((buf + block_size) - cp);
        ideal_len = need_len(dp->name_len);
        remain = dp->rec_len-ideal_len;
        if(remain >= nlen){
            dp->rec_len = ideal_len;
            cp += dp->rec_len;
            dp = (DIR_T *)cp;
            dp->inode = ino;
            dp->rec_len = remain;
            dp->name_len = (uint8_t)name_len;
            dp->file_type = (uint8_t)ftype;
            strcpy(dp->name, name);
            pip->i_mtime = now;
            pip->i_ctime = now;
            return ext2->write_block(blk, buf);
        }
    }
    return -1;
}

static int32_t ext2_rm_child(ext2_t* ext2, INODE *ip, const char *name) {
    int32_t rec_len, found, first_len, blk;
    char *cp = NULL, *precp = NULL;
    DIR_T *dp = NULL;
    uint32_t block_size = ext2_block_size(ext2);
    uint32_t blocks = ext2_dir_block_count(ext2, ip);
    char buf[EXT2_MAX_BLOCK_SIZE];

    found = 0;
    rec_len = 0;
    first_len = 0;
    for(uint32_t lbk = 0; lbk < blocks; lbk++) {
        if(ext2_get_data_block(ext2, ip, (int32_t)lbk, &blk) != 0 || blk == 0)
            return -1;
        if(ext2->read_block(blk, buf) != 0)
            return -1;
        cp = buf;
        precp = NULL;
        while(cp < (buf + block_size)) {
            dp = (DIR_T *)cp;
            if(dp->name_len == 0 || dp->rec_len < 12 || dp->rec_len < need_len(dp->name_len) ||
                    (cp + dp->rec_len) > (buf + block_size))
                break;
            if(found == 0 && dp->inode != 0 && ext2_dirent_name_equals(dp, name)) {
                //(2).1. if (first and only entry in a data block){
                if(dp->rec_len == block_size){
                    dp->inode = 0;
                    ip->i_mtime = ext2_now();
                    ip->i_ctime = ip->i_mtime;
                    return ext2->write_block(blk, buf);
                }
                //(2).2. else if LAST entry in block
                else if(precp != NULL && (cp + dp->rec_len) == (buf + block_size)){
                    dp = (DIR_T *)precp;
                    dp->rec_len = (uint16_t)(block_size - (precp - buf));
                    ip->i_mtime = ext2_now();
                    ip->i_ctime = ip->i_mtime;
                    return ext2->write_block(blk, buf);
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
            char cpbuf[EXT2_MAX_BLOCK_SIZE];
            memset(cpbuf, 0, block_size);
            memcpy(cpbuf, buf, first_len);
            memcpy(cpbuf + first_len, buf + first_len + rec_len, block_size - (first_len + rec_len));
            // Update the last entry's rec_len
            cp = cpbuf;
            while((cp + ((DIR_T *)cp)->rec_len) < (cpbuf + block_size)) {
                if(((DIR_T *)cp)->rec_len == 0) //corrupted entry, heal below
                    break;
                cp += ((DIR_T *)cp)->rec_len;
            }
            ((DIR_T *)cp)->rec_len = (uint16_t)(block_size - (cp - cpbuf));
            ip->i_mtime = ext2_now();
            ip->i_ctime = ip->i_mtime;
            return ext2->write_block(blk, cpbuf);
        }
    }
    return -1;
}

int32_t ext2_read_block(ext2_t* ext2, INODE* node, char *buf, int32_t nbytes, int32_t offset) {
    //(2) count = 0
    // avil = fileSize - OFT's offset // number of bytes still available in file.
    int32_t count_read = 0;
    char *cq = buf;
    int32_t avil = node->i_size - offset;
    int32_t blk =0, lbk = 0, start_byte = 0, remain = 0;
    uint32_t block_size = ext2_block_size(ext2);
    //(3)
    /*(4) Compute LOGICAL BLOCK number lbk and start_byte in that block from offset;
        lbk       = oftp->offset / EXT2_BLOCK_SIZE;
        start_byte = oftp->offset % EXT2_BLOCK_SIZE;*/	
    lbk = offset / (int32_t)block_size;
    start_byte = offset % (int32_t)block_size;
    if(nbytes > ((int32_t)block_size - start_byte))
        nbytes = (int32_t)block_size - start_byte;
    //(5) READ
    if(ext2_get_data_block(ext2, node, lbk, &blk) != 0)
        return -1;

    char readbuf[EXT2_MAX_BLOCK_SIZE];
    char *cp;
    if(blk == 0) {
        /* Sparse hole (lseek past EOF + write): reads as zeroes instead
         * of failing, otherwise the hole would look like EOF and hide
         * all data written beyond it. */
        memset(readbuf, 0, block_size);
        cp = readbuf + start_byte;
    }
    else if(start_byte == 0 && nbytes >= (int32_t)block_size) {
        if(ext2->read_block(blk, cq) != 0)
            return -1;
        cp = cq;
    }
    else {
        if(ext2->read_block(blk, readbuf) != 0)
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

int32_t ext2_read(ext2_t* ext2, INODE* node, char *buf, int32_t nbytes, int32_t offset) {
    char* p = buf;
    int32_t ret = nbytes;
    int32_t avil = node->i_size - offset;
    uint32_t block_size = ext2_block_size(ext2);
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
            if(max_blocks > 0 && ext2_get_data_block(ext2, node, start_lbk, &first_blk) == 0 &&
                    first_blk != 0) {
                int32_t blocks = 1;
                while(blocks < max_blocks) {
                    int32_t next_blk = 0;
                    if(ext2_get_data_block(ext2, node, start_lbk + blocks, &next_blk) != 0)
                        break;
                    if(next_blk != (first_blk + blocks))
                        break;
                    blocks++;
                }

                if(ext2_read_blocks_io(ext2, first_blk, p, (uint32_t)blocks) != 0)
                    return ret - nbytes;

                int32_t rd = blocks * (int32_t)block_size;
                nbytes -= rd;
                offset += rd;
                avil -= rd;
                p += rd;
                continue;
            }
        }

        int32_t rd = ext2_read_block(ext2, node, p, nbytes, offset);
        if(rd <= 0)
            return ret - nbytes;
        nbytes -= rd;
        offset += rd;
        avil -= rd;
        p += rd;
    }
    return ret - nbytes;
}

static INODE* get_node_by_ino(ext2_t* ext2, uint32_t ino, char* buf) {
    if(ino == 0 || ino > ext2->super.s_inodes_count)
        return NULL;
    uint32_t bgid = get_gd_index_by_ino(ext2, ino);
    ino = get_ino_in_group(ext2, ino, bgid);
    uint32_t inode_size = ext2_inode_size(ext2);
    uint32_t inodes_per_block = ext2_block_size(ext2) / inode_size;
    uint32_t offset = (ino - 1) % inodes_per_block;

    uint32_t blk = ext2->gds[bgid].bg_inode_table + ((ino - 1) / inodes_per_block);
    if(ext2->read_block(blk, buf) != 0)
        return NULL;
    return (INODE *)(buf + (offset * inode_size));
}

int32_t put_node(ext2_t* ext2, uint32_t ino, INODE *node) {
    uint32_t bgid = get_gd_index_by_ino(ext2, ino);
    ino = get_ino_in_group(ext2, ino, bgid);
    uint32_t inode_size = ext2_inode_size(ext2);
    uint32_t inodes_per_block = ext2_block_size(ext2) / inode_size;
    uint32_t offset = (ino - 1) % inodes_per_block;
    uint32_t blk = ext2->gds[bgid].bg_inode_table + ((ino - 1) / inodes_per_block);
    char buf[EXT2_MAX_BLOCK_SIZE];
    if(ext2->read_block(blk, buf) != 0)
        return -1;
    memcpy(buf + (offset * inode_size), node, sizeof(INODE));
        if(ext2->write_block(blk, buf) != 0)
                return -1;
        return ext2_flush_meta(ext2);
}

int32_t ext2_create_dir(ext2_t* ext2, uint32_t father_ino, INODE* father_inp, const char *name,
        uint16_t uid, uint16_t gid, uint16_t mode ) {
    uint32_t ino, i, blk;
    uint32_t block_size = ext2_block_size(ext2);
    uint32_t now = ext2_now();
    INODE* inp;
    char buf[EXT2_MAX_BLOCK_SIZE];

    ino = ext2_ialloc(ext2); //alloc a node id from table
    if(ino == 0)
        return -1;
    blk = ext2_balloc(ext2); //alloc a block
    if(blk == 0) //disk full: nothing committed to the parent dir yet, roll back
        goto fail_ino;

    inp = get_node_by_ino(ext2, ino, buf); //read inode from block
    if(inp == NULL)
        goto fail;
    //set inode info
    /* Force the on-disk file-type bits: the VFS only hands us the
     * permission bits, and without S_IFDIR every other ext2
     * implementation (Linux/macOS/fsck) treats the inode as garbage. */
    inp->i_mode = EXT2_S_IFDIR | (mode & 0x0FFF);
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

    if(put_node(ext2, ino, inp) != 0)
        goto fail; //write inode back to block

    /* Initialize the new directory's data block ON DISK with "." and
     * "..". The old code left the freshly allocated block holding
     * whatever bytes it previously had: the live directory tree is
     * served from vfsd's RAM nodes so everything looked fine, but
     * after a reboot the tree rebuild walked this garbage block and
     * every child created inside the directory was lost/hidden. */
    memset(buf, 0, block_size);
    DIR_T* dp = (DIR_T*)buf;
    dp->inode = ino;
    dp->rec_len = 12;
    dp->name_len = 1;
    dp->file_type = EXT2_FT_DIR;
    dp->name[0] = '.';
    dp = (DIR_T*)(buf + 12);
    dp->inode = father_ino;
    dp->rec_len = (uint16_t)(block_size - 12);
    dp->name_len = 2;
    dp->file_type = EXT2_FT_DIR;
    dp->name[0] = '.';
    dp->name[1] = '.';
    if(ext2->write_block(blk, buf) != 0)
        goto fail;

    if(write_child(ext2, father_inp, ino, name, EXT2_FT_DIR) < 0) //write dir info (name, type)
        goto fail;

    /* write_child may have grown the parent (new i_block/i_size) and
     * ".." adds a link to it: persist the parent inode, otherwise the
     * changes only live in the caller's in-memory copy. */
    father_inp->i_links_count++;
    father_inp->i_mtime = now;
    father_inp->i_ctime = now;
    /* keep bg_used_dirs_count in sync so external tools (fsck) stay happy */
    uint32_t gidx = get_gd_index_by_ino(ext2, ino);
    ext2->gds[gidx].bg_used_dirs_count++;
    mark_gd_dirty(ext2, gidx);
    if(put_node(ext2, father_ino, father_inp) != 0)
        return -1; //the dirent is already committed; only the parent's inode update was lost
    return ino;

fail: //no dirent written yet: release both allocations
    ext2_bdealloc(ext2, blk);
fail_ino:
    ext2_idealloc(ext2, ino);
    ext2_flush_meta(ext2);
    return -1;
}

int32_t ext2_create_file(ext2_t* ext2, uint32_t father_ino, INODE* father_inp, const char *name,
        uint16_t uid, uint16_t gid, uint16_t mode ) {
    uint32_t ino, i;
    uint32_t now = ext2_now();
    INODE* inp;
    char buf[EXT2_MAX_BLOCK_SIZE];

    ino = ext2_ialloc(ext2);
    if(ino == 0)
        return -1;

    inp = get_node_by_ino(ext2, ino, buf);
    if(inp == NULL)
        goto fail;
    inp->i_mode = EXT2_S_IFREG | (mode & 0x0FFF);
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
    if(put_node(ext2, ino, inp) != 0)
        goto fail;

    if(write_child(ext2, father_inp, ino, name, EXT2_FT_FILE) < 0)
        goto fail;

    /* Persist parent inode changes made by write_child (see above). */
    father_inp->i_mtime = now;
    father_inp->i_ctime = now;
    if(put_node(ext2, father_ino, father_inp) != 0)
        return -1; //the dirent is already committed; only the parent's inode update was lost
    return ino;

fail: //no dirent written yet: release the allocated inode
    ext2_idealloc(ext2, ino);
    ext2_flush_meta(ext2);
    return -1;
}

static int32_t ext2_ensure_data_block(ext2_t* ext2, INODE* node, int32_t lbk, int32_t* blk, int32_t* is_new) {
    uint32_t entries_per_block = ext2_indirect_entries(ext2);
    uint32_t indirect1[EXT2_MAX_BLOCK_SIZE / sizeof(uint32_t)];
    uint32_t indirect2[EXT2_MAX_BLOCK_SIZE / sizeof(uint32_t)];
    uint32_t indirect3[EXT2_MAX_BLOCK_SIZE / sizeof(uint32_t)];
    uint32_t block_size = ext2_block_size(ext2);
        uint32_t sectors_per_block = block_size / SECTOR_SIZE;

    if(is_new != NULL)
        *is_new = 0;

    if(lbk < 12) {
        if(node->i_block[lbk] == 0) {
            node->i_block[lbk] = ext2_balloc(ext2);
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
            node->i_block[12] = ext2_balloc(ext2);
            if(node->i_block[12] == 0)
                return -1;
                        node->i_blocks += sectors_per_block;
            if(_cached_indirect_blk != node->i_block[12]) {
                if(_cached_indirect_blk != 0 && _cached_indirect_dirty != 0) {
                    if(ext2->write_block((int32_t)_cached_indirect_blk, (char*)_cached_indirect_block) != 0)
                        return -1;
                }
                _cached_indirect_blk = node->i_block[12];
                _cached_indirect_dirty = 0;
            }
            memset(_cached_indirect_block, 0, block_size);
        }
        indirect = ext2_get_cached_indirect_block(ext2, node->i_block[12]);
        if(indirect == NULL)
            return -1;
        if(indirect[lbk - 12] == 0) {
            indirect[lbk - 12] = ext2_balloc(ext2);
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

        if(node->i_block[13] == 0) {
            node->i_block[13] = ext2_balloc(ext2);
            if(node->i_block[13] == 0)
                return -1;
                        node->i_blocks += sectors_per_block;
            memset(indirect1, 0, block_size);
            if(ext2->write_block(node->i_block[13], (char*)indirect1) != 0)
                return -1;
        }
        if(ext2->read_block(node->i_block[13], (char*)indirect1) != 0)
            return -1;
        if(indirect1[num] == 0) {
            indirect1[num] = ext2_balloc(ext2);
            if(indirect1[num] == 0)
                return -1;
                        node->i_blocks += sectors_per_block;
            memset(indirect2, 0, block_size);
            if(ext2->write_block((int32_t)indirect1[num], (char*)indirect2) != 0)
                return -1;
            if(ext2->write_block(node->i_block[13], (char*)indirect1) != 0)
                return -1;
        }
        if(ext2->read_block((int32_t)indirect1[num], (char*)indirect2) != 0)
            return -1;
        if(indirect2[pos] == 0) {
            indirect2[pos] = ext2_balloc(ext2);
            if(indirect2[pos] == 0)
                return -1;
                        node->i_blocks += sectors_per_block;
            if(ext2->write_block((int32_t)indirect1[num], (char*)indirect2) != 0)
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
            node->i_block[14] = ext2_balloc(ext2);
            if(node->i_block[14] == 0)
                return -1;
                        node->i_blocks += sectors_per_block;
            memset(indirect1, 0, block_size);
            if(ext2->write_block(node->i_block[14], (char*)indirect1) != 0)
                return -1;
        }
        if(ext2->read_block(node->i_block[14], (char*)indirect1) != 0)
            return -1;
        if(indirect1[num1] == 0) {
            indirect1[num1] = ext2_balloc(ext2);
            if(indirect1[num1] == 0)
                return -1;
                        node->i_blocks += sectors_per_block;
            memset(indirect2, 0, block_size);
            if(ext2->write_block((int32_t)indirect1[num1], (char*)indirect2) != 0)
                return -1;
            if(ext2->write_block(node->i_block[14], (char*)indirect1) != 0)
                return -1;
        }
        if(ext2->read_block((int32_t)indirect1[num1], (char*)indirect2) != 0)
            return -1;
        if(indirect2[num2] == 0) {
            indirect2[num2] = ext2_balloc(ext2);
            if(indirect2[num2] == 0)
                return -1;
                        node->i_blocks += sectors_per_block;
            memset(indirect3, 0, block_size);
            if(ext2->write_block((int32_t)indirect2[num2], (char*)indirect3) != 0)
                return -1;
            if(ext2->write_block((int32_t)indirect1[num1], (char*)indirect2) != 0)
                return -1;
        }
        if(ext2->read_block((int32_t)indirect2[num2], (char*)indirect3) != 0)
            return -1;
        if(indirect3[pos] == 0) {
            indirect3[pos] = ext2_balloc(ext2);
            if(indirect3[pos] == 0)
                return -1;
                        node->i_blocks += sectors_per_block;
            if(ext2->write_block((int32_t)indirect2[num2], (char*)indirect3) != 0)
                return -1;
            if(is_new != NULL)
                *is_new = 1;
        }
        *blk = (int32_t)indirect3[pos];
        return 0;
    }

    return -1;
}

int32_t ext2_write(ext2_t* ext2, INODE* node, const char *data, int32_t nbytes, int32_t offset) {
    static char buf[EXT2_MAX_BLOCK_SIZE];
    const char *cq = data;
    char *cp;
    uint32_t now = ext2_now();
    uint32_t block_size = ext2_block_size(ext2);
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
                if(ext2_ensure_data_block(ext2, node, lbk + run_blocks, &cur_blk, NULL) != 0)
                    break;
                if(run_blocks == 0)
                    first_blk = cur_blk;
                else if(cur_blk != (first_blk + run_blocks))
                    break;
                run_blocks++;
            }

            if(run_blocks > 0 && ext2_write_blocks_io(ext2, first_blk, cq, (uint32_t)run_blocks) == 0) {
                int32_t wrote = run_blocks * (int32_t)block_size;
                nbytes_copy += wrote;
                nbytes -= wrote;
                offset += wrote;
                cq += wrote;
                if(offset > (int32_t)node->i_size)
                    node->i_size = offset;
                continue;
            }

            if(ext2_ensure_data_block(ext2, node, lbk, &blk, NULL) != 0)
                return nbytes_copy;
            if(ext2->write_block(blk, (char*)cq) != 0)
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
            if(ext2_ensure_data_block(ext2, node, lbk, &blk, &fresh) != 0)
                return nbytes_copy;
            /* A freshly allocated block may still hold stale data from a
             * previously deleted file (freed blocks are no longer zeroed
             * on disk): start from zeroes instead of reading it back. */
            if(fresh != 0)
                memset(buf, 0, block_size);
            else if(ext2->read_block(blk, buf) != 0)
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

            if(ext2->write_block(blk, buf) != 0)
                return nbytes_copy;
        }
    }
    if(nbytes_copy > 0) {
        node->i_mtime = now;
        node->i_ctime = now;
    }
    return nbytes_copy;
}

static uint32_t search(ext2_t* ext2, INODE *ip, const char *name, DIR_T* dirp) {
    char *cp;
    DIR_T  *dp;
    uint32_t block_size = ext2_block_size(ext2);
    uint32_t blocks = ext2_dir_block_count(ext2, ip);
    char buf[EXT2_MAX_BLOCK_SIZE];
    if(dirp != NULL)
        memset(dirp, 0, sizeof(DIR_T));

    for(uint32_t lbk = 0; lbk < blocks; lbk++) {
        if(ext2_read_inode_block(ext2, ip, lbk, buf) != 0)
            return 0;
        dp = (DIR_T *)buf;
        cp = buf;
        while (cp < (buf + block_size)){
            if(dp->name_len == 0 || dp->rec_len < 12 || dp->rec_len < need_len(dp->name_len) ||
                    (cp + dp->rec_len) > (buf + block_size))
                break;
            if(dp->inode != 0 && ext2_dirent_name_equals(dp, name)) {
                if(dirp != NULL) {
                    memcpy(dirp, dp, sizeof(DIR_T));
                    if(dp->name_len < sizeof(dirp->name))
                        dirp->name[dp->name_len] = 0;
                }
                return dp->inode;
            }
            cp += dp->rec_len;
            dp = (DIR_T *)cp;
        }
    }
    return 0;
}

#define MAX_DIR_DEPTH 32
static int32_t split_fname(const char* filename, str_t* name[]) {
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

uint32_t ext2_ino_by_fname(ext2_t* ext2, const char* filename, DIR_T* dirp) {
    char buf[EXT2_MAX_BLOCK_SIZE];
    uint32_t depth, i, ino;
    str_t* name[MAX_DIR_DEPTH];
    INODE *ip;

    if(strcmp(filename, "/") == 0) {
        if(dirp != NULL) {
            memset(dirp, 0, sizeof(DIR_T));
            dirp->inode = 2;
            dirp->file_type = EXT2_FT_DIR;
            dirp->name_len = 1;
            dirp->name[0] = '/';
            dirp->name[1] = 0;
        }
        return 2; //ino 2 for root;
    }
    depth = split_fname(filename, name);

    ino = 2; // ext2 root inode
    ip = get_node_by_ino(ext2, ino, buf);
    if(ip != NULL) {
        for (i=0; i<depth; i++) {
            ino = search(ext2, ip, CS(name[i]), dirp);
            if (ino == 0) {
                break;
            }
            ip = get_node_by_ino(ext2, ino, buf);
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

int32_t ext2_node_by_fname(ext2_t* ext2, const char* filename, INODE* inode) {
    uint32_t ino = ext2_ino_by_fname(ext2, filename, NULL);
    if(ino == 0)
        return -1;
    return ext2_node_by_ino(ext2, ino, inode);
}

int32_t ext2_truncate(ext2_t* ext2, uint32_t ino, INODE* node) {
    uint32_t now = ext2_now();

    if(ext2_free_inode_data(ext2, node) != 0)
        return -1;
    node->i_mtime = now;
    node->i_ctime = now;
    node->i_dtime = 0;
    if(ino != 0 && put_node(ext2, ino, node) != 0)
        return -1;
    return 0;
}

static int32_t ext2_remove_path(ext2_t* ext2, const char* fname, int32_t want_dir) {
    char buf[EXT2_MAX_BLOCK_SIZE];
    char inode_buf[EXT2_MAX_BLOCK_SIZE];
    char dir[FS_FULL_NAME_MAX];
    char name[FS_FULL_NAME_MAX];
    DIR_T dirp;
    uint32_t now = ext2_now();
    int32_t is_dir;

    vfs_dir_name(fname, dir, FS_FULL_NAME_MAX);
    vfs_file_name(fname, name, FS_FULL_NAME_MAX);
    if(name[0] == 0 || strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        return -1;

    uint32_t fino = ext2_ino_by_fname(ext2, dir, NULL);
    if(fino == 0)
        return -1;

    INODE* fnode = get_node_by_ino(ext2, fino, buf);
    if(fnode == NULL)
        return -1;

    uint32_t ino = search(ext2, fnode, name, &dirp);
    if(ino == 0)
        return -1;

    INODE* node = get_node_by_ino(ext2, ino, inode_buf);
    if(node == NULL)
        return -1;

    is_dir = ((node->i_mode & 0xF000) == EXT2_S_IFDIR) || dirp.file_type == EXT2_FT_DIR;
    if(want_dir) {
        if(!is_dir || ext2_dir_is_empty(ext2, node) != 1)
            return -1;
    }
    else if(is_dir) {
        return -1;
    }

    if(ext2_rm_child(ext2, fnode, name) != 0)
        return -1;
    if(want_dir && fnode->i_links_count > 0)
        fnode->i_links_count--;
    fnode->i_mtime = now;
    fnode->i_ctime = now;
    if(put_node(ext2, fino, fnode) != 0)
        return -1;

    if(!want_dir && node->i_links_count > 1) {
        node->i_links_count--;
        node->i_ctime = now;
        node->i_dtime = 0;
        return put_node(ext2, ino, node);
    }

    if(ext2_free_inode_data(ext2, node) != 0)
        return -1;
    node->i_links_count = 0;
    node->i_dtime = now;
    node->i_ctime = now;
    node->i_mtime = now;
    if(put_node(ext2, ino, node) != 0)
        return -1;
    if(ext2_idealloc(ext2, ino) != 0)
        return -1;
    if(want_dir) { //keep bg_used_dirs_count in sync (see ext2_create_dir)
        uint32_t gidx = get_gd_index_by_ino(ext2, ino);
        if(ext2->gds[gidx].bg_used_dirs_count > 0)
            ext2->gds[gidx].bg_used_dirs_count--;
        mark_gd_dirty(ext2, gidx);
    }
        if(ext2_flush_meta(ext2) != 0)
                return -1;
    return 0;
}

int32_t ext2_unlink(ext2_t* ext2, const char* fname) {
    return ext2_remove_path(ext2, fname, 0);
}

int32_t ext2_node_by_ino(ext2_t* ext2, uint32_t ino, INODE* node) {
    char buf[EXT2_MAX_BLOCK_SIZE];
    INODE* p = get_node_by_ino(ext2, ino, buf);
    if(p == NULL)
        return -1;
    memcpy(node, p, sizeof(INODE));
    return 0;
}

int32_t ext2_rmdir(ext2_t* ext2, const char *fname) {
    return ext2_remove_path(ext2, fname, 1);
}

static inline int32_t get_gd_num(ext2_t* ext2) {
    uint32_t count = ext2->super.s_blocks_count - ext2->super.s_first_data_block;
    int32_t ret = (int32_t)(count / ext2->super.s_blocks_per_group);
    if((count % ext2->super.s_blocks_per_group) != 0)
        ret++;
    return ret;
}

static int32_t get_gds(ext2_t* ext2) {
    int32_t gd_size = sizeof(GD);
    uint32_t block_size = ext2_block_size(ext2);
    uint32_t gdt_block = ext2_gdt_start_block(ext2);
    ext2->group_num = get_gd_num(ext2);
    ext2->gds = (GD*)malloc(gd_size * ext2->group_num);
    if(ext2->gds == NULL)
        return -1;

    int32_t gd_num = (int32_t)(block_size / gd_size);
    int32_t index = 0;
    while(true) {
        char buf[EXT2_MAX_BLOCK_SIZE];
        if(ext2->read_block((int32_t)gdt_block, buf) != 0)
            return -1;
        for(int32_t j = 0; j < gd_num; j++) {
            memcpy(&ext2->gds[index], buf + (j * gd_size), gd_size);
            index++;
            if(index >= ext2->group_num)
                return 0;
        }
        gdt_block++;
    }
    return 0;
}

int32_t ext2_init(ext2_t* ext2, read_block_func_t read_block, write_block_func_t write_block, uint32_t buffer_size) {
    return ext2_init_ex2(ext2, read_block, NULL, write_block, NULL, buffer_size);
}

int32_t ext2_init_ex(ext2_t* ext2, read_block_func_t read_block, read_blocks_func_t read_blocks, write_block_func_t write_block, uint32_t buffer_size) {
    return ext2_init_ex2(ext2, read_block, read_blocks, write_block, NULL, buffer_size);
}

int32_t ext2_init_ex2(ext2_t* ext2, read_block_func_t read_block, read_blocks_func_t read_blocks,
        write_block_func_t write_block, write_blocks_func_t write_blocks, uint32_t buffer_size) {
    char buf[EXT2_MAX_BLOCK_SIZE];
    ext2->read_block = read_block;
    ext2->read_blocks = read_blocks;
    ext2->write_block = write_block;
    ext2->write_blocks = write_blocks;
    ext2->gds = NULL;
    ext2->next_alloc_block = 0;
        ext2->dirty_gds = NULL;
        ext2->dirty_super = 0;
        ext2->cached_block_bitmap = NULL;
    ext2->cached_block_bitmap_blk = 0;
    ext2->cached_block_bitmap_dirty = 0;
        ext2->cached_inode_bitmap = NULL;
    ext2->cached_inode_bitmap_blk = 0;
    ext2->cached_inode_bitmap_dirty = 0;
        _cached_block_bitmap_blk = 0;
        _cached_block_bitmap_dirty = 0;
        _cached_inode_bitmap_blk = 0;
        _cached_inode_bitmap_dirty = 0;
        _cached_indirect_blk = 0;
        _cached_indirect_dirty = 0;
        memset(_cached_indirect_block, 0, sizeof(_cached_indirect_block));

        if(_cached_block_bitmap == NULL)
                _cached_block_bitmap = malloc(EXT2_MAX_BLOCK_SIZE);
        if(_cached_inode_bitmap == NULL)
                _cached_inode_bitmap = malloc(EXT2_MAX_BLOCK_SIZE);
        if(_cached_block_bitmap == NULL || _cached_inode_bitmap == NULL) {
                free(_cached_block_bitmap);
                free(_cached_inode_bitmap);
                _cached_block_bitmap = NULL;
                _cached_inode_bitmap = NULL;
                return -1;
        }
    sd_set_block_size(EXT2_DEFAULT_BLOCK_SIZE);

    //read super block
    if(ext2->read_block(1, buf) != 0)
        return -1;
    memcpy(&ext2->super, buf, sizeof(SUPER));
    if(ext2_validate_super(ext2) != 0)
        return -1;
    if(sd_set_block_size(ext2_block_size(ext2)) != 0)
        return -1;
    if(get_gds(ext2) != 0)
        return -1;
        ext2->dirty_gds = (uint8_t*)calloc((size_t)ext2->group_num, sizeof(uint8_t));
        if(ext2->dirty_gds == NULL)
                return -1;
    ext2->next_alloc_block = ext2->super.s_first_data_block;
    sd_set_max_sector_index(ext2->super.s_blocks_count * (ext2_block_size(ext2) / SECTOR_SIZE));
    sd_set_buffer_size(buffer_size);
    return 0;
}

void ext2_quit(ext2_t* ext2) {
        (void)ext2_flush_meta(ext2);
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
        free(ext2->dirty_gds);
    free(ext2->gds);
}

void* ext2_readfile(ext2_t* ext2, const char* fname, int32_t* size) {
    void* ret = NULL;
    uint32_t block_size = ext2_block_size(ext2);
    if(size != NULL)
        *size = -1;

    uint32_t ino = ext2_ino_by_fname(ext2, fname, NULL);
    if(ino > 0) {
        INODE inode;
        if(ext2_node_by_ino(ext2, ino, &inode) != 0) {
            return ret;
        }

        char *data = (char*)malloc(inode.i_size + 1);
        if(data != NULL) {
            ret = data;
            uint32_t rd = 0;
            while(1) {
                int sz = ext2_read(ext2, &inode, data, (int32_t)block_size, (int32_t)rd);
                if(sz <= 0 || rd >= inode.i_size)
                    break;
                data += sz;
                rd += sz;
            }
            if(size != NULL)
                *size = rd;
        }
    }
    return ret;
}
