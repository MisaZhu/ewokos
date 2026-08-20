#include "ext3head.h"
#include "ext3read.h"
#include "partition.h"
#include <kstring.h>
#include <mm/kmalloc.h>
#include <dev/sd.h>
#include <stddef.h>

#define SECTOR_SIZE     512

/*
 * Minimal read-only ext3 reader for the kernel boot path (kernel config,
 * /sbin/init).  Ported from the userspace ext3 library (system/basic/
 * libs/ext3), read path only:
 *
 *  - the journal is never written or replayed; EXT3_FEATURE_INCOMPAT_RECOVER
 *    is accepted so a filesystem that was not cleanly unmounted still reads
 *    (recently written data may be stale until sdfsd recovers the journal).
 *  - directories are scanned linearly across all their data blocks, so
 *    HTree (dir_index) directories read correctly: the dx_root/dx_node
 *    index blocks carry a name_len==0 fake dirent that ends the scan of
 *    that block, real entries live in the leaf blocks.
 *  - block pointers resolve through direct, single- and double-indirect
 *    blocks (triple-indirect is not needed for boot-time files).
 *
 * The boot path runs on the tiny per-core SVC stack before the scheduler
 * starts, so every large buffer and the filesystem context live in BSS.
 * Boot-time card access is single-threaded, sharing them is safe.
 */
static uint8_t  _io_buf[EXT3_MAX_BLOCK_SIZE] __attribute__((aligned(4)));   /* super/GDT/inode-table reads */
static uint8_t  _dir_buf[EXT3_MAX_BLOCK_SIZE] __attribute__((aligned(4)));  /* directory block scan */
static uint8_t  _file_buf[EXT3_MAX_BLOCK_SIZE] __attribute__((aligned(4))); /* file data staging */
static uint32_t _ind_buf[EXT3_MAX_BLOCK_SIZE / sizeof(uint32_t)];           /* single-indirect */
static uint32_t _dind_buf1[EXT3_MAX_BLOCK_SIZE / sizeof(uint32_t)];         /* double-indirect level 1 */
static uint32_t _dind_buf2[EXT3_MAX_BLOCK_SIZE / sizeof(uint32_t)];         /* double-indirect level 2 */
static ext3_t   _ext3;                                                      /* fs context (holds 1KB super) */

static partition_t _partition;
static uint32_t _ext3_block_size = EXT3_DEFAULT_BLOCK_SIZE;
static int32_t  _fs_type = -1; /* ext3_probe_fs() result cache */

/*
 * Some kernel BSPs only provide the legacy single-sector SD interface.
 * Treat multi-block read as an optional optimization and fall back cleanly
 * when the platform does not implement it.
 */
extern int32_t sd_dev_read_blocks(int32_t sector, void* buf, uint32_t count)
        __attribute__((weak));

static int32_t sd_read_sector(int32_t sector, void* buf) {
    for (int32_t retry = 0; retry < 8; ++retry) {
        if (sd_dev_read(sector) != 0) {
            continue;
        }
        if (sd_dev_read_done(buf) == 0) {
            return 0;
        }
    }
    return -1;
}

static int32_t sd_read_sectors(int32_t sector, void* buf, uint32_t count) {
        if(count == 0)
                return 0;

        if(sd_dev_read_blocks != NULL) {
                for (int32_t retry = 0; retry < 4; ++retry) {
                        if (sd_dev_read_blocks(sector, buf, count) == 0) {
                                return 0;
                        }
                }
        }

        return -1;
}

static int32_t sd_read(int32_t block, void* buf) {
    int32_t n = (int32_t)(_ext3_block_size / SECTOR_SIZE);
    int32_t sector = block * n + (int32_t)_partition.start_sector;

        if(sd_read_sectors(sector, buf, (uint32_t)n) == 0)
                return 0;

        char* p = (char*)buf;
        while(n > 0) {
                if(sd_read_sector(sector, p) != 0) {
                        return -1;
                }
                sector++;
                p += 512;
                n--;
        }
        return 0;
}

static int32_t ext3_validate_super(ext3_t* ext3) {
    uint32_t unsupported_incompat = ext3->super.s_feature_incompat &
        ~(EXT3_FEATURE_INCOMPAT_FILETYPE | EXT3_FEATURE_INCOMPAT_RECOVER);
    uint32_t unsupported_ro = ext3->super.s_feature_ro_compat &
        ~(EXT3_FEATURE_RO_COMPAT_SPARSE_SUPER | EXT3_FEATURE_RO_COMPAT_LARGE_FILE);
    uint32_t unsupported_compat = ext3->super.s_feature_compat &
        ~(EXT3_FEATURE_COMPAT_HAS_JOURNAL | EXT3_FEATURE_COMPAT_EXT_ATTR |
          EXT3_FEATURE_COMPAT_RESIZE_INODE | EXT3_FEATURE_COMPAT_DIR_INDEX);

    if(ext3->super.s_magic != EXT3_SUPER_MAGIC)
        return -1;
    uint32_t block_size = ext3_block_size(ext3);
    if(block_size != 1024 && block_size != 2048 && block_size != 4096)
        return -1;
    if(ext3_inode_size(ext3) < sizeof(EXT3_INODE))
        return -1;
    /* whole 32-byte group descriptors only */
    if(ext3->super.s_desc_size != 0 && ext3->super.s_desc_size != sizeof(EXT3_GD))
        return -1;
    /* RECOVER is accepted: it just means the previous mount did not finish
     * cleanly and the journal should be replayed (read-only mount skips
     * replay; plain ext2 images carry none of these features and validate
     * here too, the probe decides which reader read_fs() calls) */
    if(unsupported_incompat != 0 || unsupported_ro != 0 || unsupported_compat != 0)
        return -1;
    return 0;
}

static inline uint32_t get_gd_index_by_ino(ext3_t* ext3, uint32_t ino) {
    return (ino-1) / ext3->super.s_inodes_per_group;
}

static inline int32_t get_gd_num(ext3_t* ext3) {
    uint32_t count = ext3->super.s_blocks_count - ext3->super.s_first_data_block;
    int32_t ret = (int32_t)(count / ext3->super.s_blocks_per_group);
    if((count % ext3->super.s_blocks_per_group) != 0)
        ret++;
    return ret;
}

static int32_t get_gds(ext3_t* ext3) {
    uint32_t gd_size = sizeof(EXT3_GD);
    uint32_t block_size = ext3_block_size(ext3);
    uint32_t gdt_block = ext3_gdt_start_block(ext3);
    ext3->group_num = get_gd_num(ext3);
    ext3->gds = (EXT3_GD*)kmalloc(gd_size * (uint32_t)ext3->group_num);
    if (ext3->gds == NULL) {
        return -1;
    }

    int32_t gd_num = (int32_t)(block_size / gd_size);
    int32_t index = 0;
    while(1) {
        if (ext3->read_block((int32_t)gdt_block, (char*)_io_buf) != 0) {
            return -1;
        }
        for(int32_t j=0; j<gd_num; j++) {
            memcpy(&ext3->gds[index], _io_buf+(j*gd_size), gd_size);
            index++;
            if(index >= ext3->group_num)
                return 0;
        }
        gdt_block++;
    }
    return 0;
}

static int32_t ext3_init(ext3_t* ext3) {
    memset(&_partition, 0, sizeof(partition_t));
    _partition.start_sector = get_rootfs_entry(sd_read_sector);

    ext3->read_block = sd_read;
    ext3->gds = NULL;
    _ext3_block_size = EXT3_DEFAULT_BLOCK_SIZE;

    if (ext3->read_block(1, (char*)_io_buf) != 0) {
        return -1;
    }
    memcpy(&ext3->super, _io_buf, sizeof(EXT3_SUPER));
    if (ext3_validate_super(ext3) != 0) {
        return -1;
    }
    _ext3_block_size = ext3_block_size(ext3);
    if (get_gds(ext3) != 0) {
        return -1;
    }

    return 0;
}

static void ext3_quit(ext3_t* ext3) {
    if(ext3->gds != NULL) {
        kfree(ext3->gds);
        ext3->gds = NULL;
    }
}

/* logical file block -> physical block: direct, single- and
 * double-indirect (0 = sparse hole, -1 = out of supported range/error) */
static int32_t ext3_get_data_block(ext3_t* ext3, EXT3_INODE* node, int32_t lbk, int32_t* blk) {
    uint32_t entries_per_block = ext3_indirect_entries(ext3);

    *blk = 0;
    if(lbk < 0)
        return -1;
    if(lbk < 12) {
        *blk = (int32_t)node->i_block[lbk];
        return 0;
    }
    if(lbk < (int32_t)(entries_per_block + 12)) {
        if(node->i_block[12] == 0)
            return 0;
        if(ext3->read_block((int32_t)node->i_block[12], (char*)_ind_buf) != 0)
            return -1;
        *blk = (int32_t)_ind_buf[lbk - 12];
        return 0;
    }

    int32_t count = lbk - 12 - (int32_t)entries_per_block;
    if(count >= (int32_t)(entries_per_block * entries_per_block))
        return -1; /* triple-indirect: beyond boot-time file sizes */
    int32_t num = count / (int32_t)entries_per_block;
    int32_t pos_offset = count % (int32_t)entries_per_block;
    if(node->i_block[13] == 0)
        return 0;
    if(ext3->read_block((int32_t)node->i_block[13], (char*)_dind_buf1) != 0)
        return -1;
    if(_dind_buf1[num] == 0)
        return 0;
    if(ext3->read_block((int32_t)_dind_buf1[num], (char*)_dind_buf2) != 0)
        return -1;
    *blk = (int32_t)_dind_buf2[pos_offset];
    return 0;
}

static EXT3_INODE* get_node_by_ino(ext3_t* ext3, uint32_t ino) {
    if(ino == 0 || ino > ext3->super.s_inodes_count)
        return NULL;
    uint32_t bgid = get_gd_index_by_ino(ext3, ino);
    if(bgid >= (uint32_t)ext3->group_num)
        return NULL;
    uint32_t ino_in_group = ino - (bgid * ext3->super.s_inodes_per_group);
    uint32_t inode_size = ext3_inode_size(ext3);
    uint32_t inodes_per_block = ext3_block_size(ext3) / inode_size;
    uint32_t offset = (ino_in_group - 1) % inodes_per_block;
    uint32_t blk = ext3->gds[bgid].bg_inode_table + ((ino_in_group - 1) / inodes_per_block);
    if(ext3->read_block((int32_t)blk, (char*)_io_buf) != 0)
        return NULL;
    return (EXT3_INODE *)(_io_buf + (offset * inode_size));
}

static int32_t ext3_node_by_ino(ext3_t* ext3, uint32_t ino, EXT3_INODE* node) {
    EXT3_INODE* p = get_node_by_ino(ext3, ino);
    if(p == NULL)
        return -1;
    memcpy(node, p, sizeof(EXT3_INODE));
    return 0;
}

static inline int32_t need_len(int32_t len) {
    return 4 * ((8 + len + 3) / 4);
}

static uint32_t ext3_search(ext3_t* ext3, EXT3_INODE *ip, const char *name) {
    char *cp;
    EXT3_DIR *dp;
    uint32_t block_size = ext3_block_size(ext3);
    uint32_t blocks = ((uint32_t)ip->i_size + block_size - 1) / block_size;
    uint32_t name_len = strlen(name);

    /* scan every directory block (direct or indirect): HTree index blocks
     * end their block's scan via the name_len==0 fake dirent, so indexed
     * directories need no special casing */
    for (uint32_t lbk=0; lbk<blocks; lbk++){
        int32_t blk = 0;
        if(ext3_get_data_block(ext3, ip, (int32_t)lbk, &blk) != 0)
            return 0;
        if(blk == 0)
            continue;
        if(ext3->read_block(blk, (char*)_dir_buf) != 0)
            return 0;
        dp = (EXT3_DIR *)_dir_buf;
        cp = (char*)_dir_buf;
        while (cp < ((char*)_dir_buf + block_size)){
            if(dp->name_len == 0 || dp->rec_len < 12 ||
                    dp->rec_len < need_len(dp->name_len) ||
                    (cp + dp->rec_len) > ((char*)_dir_buf + block_size))
                break;
            if(dp->inode != 0 && dp->name_len == name_len &&
                    memcmp(dp->name, (void*)name, name_len) == 0){
                return dp->inode;
            }
            cp += dp->rec_len;
            dp = (EXT3_DIR *)cp;
        }
    }
    return 0;
}

#define MAX_COMP_NAME 63

static uint32_t ext3_ino_by_fname(ext3_t* ext3, const char* filename) {
    uint32_t ino = 2; /* ino 2 for root */
    const char* p = filename;
    char comp[MAX_COMP_NAME+1];

    while(*p != 0 && ino != 0) {
        uint32_t u = 0;
        while(*p == '/')
            p++;
        while(*p != 0 && *p != '/' && u < MAX_COMP_NAME)
            comp[u++] = *p++;
        comp[u] = 0;
        if(u == 0) {
            if(*p == 0)
                break;
            p++;
            continue;
        }
        EXT3_INODE *ip = get_node_by_ino(ext3, ino);
        if(ip == NULL)
            return 0;
        ino = ext3_search(ext3, ip, comp);
    }
    return ino;
}

static int32_t ext3_read_data(ext3_t* ext3, EXT3_INODE* node, char *buf, int32_t offset) {
    int32_t count_read = 0;
    int32_t avil = (int32_t)node->i_size - offset;
    uint32_t block_size = ext3_block_size(ext3);

    while(avil > 0) {
        int32_t lbk = offset / (int32_t)block_size;
        int32_t start_byte = offset % (int32_t)block_size;
        int32_t chunk = (int32_t)block_size - start_byte;
        if(chunk > avil)
            chunk = avil;

        int32_t blk = 0;
        if(ext3_get_data_block(ext3, node, lbk, &blk) != 0)
            break;
        if(blk == 0) {
            memset(buf, 0, (uint32_t)chunk); /* sparse hole reads as zeros */
        }
        else {
            if(ext3->read_block(blk, (char*)_file_buf) != 0)
                break;
            memcpy(buf, _file_buf + start_byte, (uint32_t)chunk);
        }
        buf += chunk;
        offset += chunk;
        count_read += chunk;
        avil -= chunk;
    }
    return count_read;
}

static void* ext3_readfile(ext3_t* ext3, const char* fname, int32_t* size) {
    void* ret = NULL;
    if(size != NULL)
        *size = -1;

    uint32_t ino = ext3_ino_by_fname(ext3, fname);
    if(ino > 0) {
        EXT3_INODE inode;
        if(ext3_node_by_ino(ext3, ino, &inode) != 0) {
            return ret;
        }

        char *data = (char*)kmalloc(inode.i_size+1); //one more byte for string end.
        if(data != NULL) {
            ret = data;
            int32_t rd = ext3_read_data(ext3, &inode, data, 0);
            data[rd] = 0;
            if(size != NULL)
                *size = rd;
        }
    }
    return ret;
}

void* sd_read_ext3(const char* fname, int32_t* size) {
    ext3_t* ext3 = &_ext3;
    if (ext3_init(ext3) != 0) {
        if (size != NULL) {
            *size = -1;
        }
        return NULL;
    }
    void* ret = ext3_readfile(ext3, fname, size);
    ext3_quit(ext3);
    return ret;
}

int32_t ext3_probe_fs(void) {
    if(_fs_type >= 0)
        return _fs_type;
    _fs_type = EXT3_PROBE_NONE;

    partition_t p;
    memset(&p, 0, sizeof(partition_t));
    p.start_sector = get_rootfs_entry(sd_read_sector);

    /* the superblock sits at byte offset 1024 of the partition
     * (sectors +2/+3), whatever the filesystem block size */
    if(sd_read_sector((int32_t)p.start_sector + 2, (char*)_io_buf) != 0)
        return _fs_type;
    if(sd_read_sector((int32_t)p.start_sector + 3, (char*)_io_buf + SECTOR_SIZE) != 0)
        return _fs_type;

    EXT3_SUPER* super = (EXT3_SUPER*)_io_buf;
    if(super->s_magic == EXT3_SUPER_MAGIC) {
        /* ext3 first: a usable internal journal makes it ext3 */
        if((super->s_feature_compat & EXT3_FEATURE_COMPAT_HAS_JOURNAL) != 0 &&
                super->s_journal_inum != 0 &&
                (super->s_feature_incompat & EXT3_FEATURE_INCOMPAT_JOURNAL_DEV) == 0)
            _fs_type = EXT3_PROBE_EXT3;
        else
            _fs_type = EXT3_PROBE_EXT2;
    }
    return _fs_type;
}
