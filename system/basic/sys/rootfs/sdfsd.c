#include <stdlib.h>
#include <unistd.h>
#include <ewoksys/wait.h>
#include <string.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <ewoksys/klog.h>
#include <sysinfo.h>
#include <fcntl.h>
#include <sd/sd.h>
#include <ext3/ext3fs.h>
#include <stdio.h>
#include <bsp/bsp_sd.h>

#define SD_BUFFER_SIZE (1024*1024*64) //64M buffer size

/*
 * Root filesystem driver for the sd card, serving both ext2 and ext3.
 *
 * At startup ext3_probe() classifies the filesystem: ext3 (a usable
 * internal journal) is checked FIRST and preferred; a filesystem
 * without a journal is mounted as plain ext2.  Both cases run through
 * the ext3 library: on ext3 every metadata update is journaled and
 * made crash-safe by ext3_commit(); on ext2 the same code paths fall
 * back to direct synchronous writes (ext2 compatible mode), giving
 * exactly the old ext2 driver's behaviour.
 *
 * ext3_commit() is issued after every metadata-modifying operation
 * (create, unlink, set, open(O_TRUNC), flush, close) so a crash at
 * any point is repaired by replaying the journal at the next mount.
 * File DATA is written directly (ordered data mode): it is always on
 * the card before the metadata referencing it is committed.
 */

/*
 * Inode cache keyed by vfs node, shared by all fds of the same file.
 * The old per-(fd,pid,node) entries each held a private copy of the
 * inode: two fds writing the same file diverged (stale i_size/i_block),
 * and the later flush overwrote the earlier one on disk, losing data.
 * refs tracks driver-side fd references: +1 per open (FS_CMD_OPEN) and
 * per fork-inherited fd (FS_CMD_DUP), -1 per close (FS_CMD_CLOSE).
 */
typedef struct inode_cache {
    int refs;
    ewokos_addr_t node;
    uint32_t ino;
    uint8_t dirty;
    EXT3_INODE inode;
    struct inode_cache* next;
} inode_cache_t;

static inode_cache_t* _inode_cache = NULL;

static void set_fsinfo_stat(node_stat_t* stat, EXT3_INODE* inode);
static void set_inode_stat(node_stat_t* stat, EXT3_INODE* inode);

static int32_t sdext_sd_read_blocks(int32_t block, void* buf, uint32_t count) {
    return sd_read_blocks(block, buf, count);
}

static int32_t sdext_sd_write_blocks(int32_t block, const void* buf, uint32_t count) {
    return sd_write_blocks(block, buf, count);
}

static inode_cache_t* inode_cache_find_by_node(ewokos_addr_t node) {
    inode_cache_t* entry = _inode_cache;

    while(entry != NULL) {
        if(entry->node == node)
            return entry;
        entry = entry->next;
    }
    return NULL;
}

static inode_cache_t* inode_cache_get(ext3_t* ext3, ewokos_addr_t node, uint32_t ino) {
    inode_cache_t* entry = inode_cache_find_by_node(node);

    if(entry != NULL)
        return entry;

    entry = (inode_cache_t*)calloc(1, sizeof(inode_cache_t));
    if(entry == NULL)
        return NULL;

    if(ext3_node_by_ino(ext3, ino, &entry->inode) != 0) {
        free(entry);
        return NULL;
    }

    entry->refs = 1;
    entry->node = node;
    entry->ino = ino;
    entry->next = _inode_cache;
    _inode_cache = entry;
    return entry;
}

/* Called once per file open: takes one driver-side reference. refresh!=0
 * (O_TRUNC just rewrote the on-disk inode) forces reloading the shared
 * state from the freshly truncated inode. */
static inode_cache_t* inode_cache_seed(ewokos_addr_t node, uint32_t ino, const EXT3_INODE* inode, int refresh) {
    inode_cache_t* entry = inode_cache_find_by_node(node);

    if(entry != NULL) {
        entry->refs++;
        if(refresh != 0) {
            memcpy(&entry->inode, inode, sizeof(EXT3_INODE));
            entry->ino = ino;
            entry->dirty = 0;
        }
        return entry;
    }

    entry = (inode_cache_t*)calloc(1, sizeof(inode_cache_t));
    if(entry == NULL)
        return NULL;
    entry->refs = 1;
    entry->node = node;
    entry->ino = ino;
    memcpy(&entry->inode, inode, sizeof(EXT3_INODE));
    entry->next = _inode_cache;
    _inode_cache = entry;
    return entry;
}

static int inode_cache_flush_entry(ext3_t* ext3, inode_cache_t* entry, fsinfo_t* info) {
    if(entry == NULL)
        return -1;

    if(entry->dirty != 0) {
        if(ext3_put_node(ext3, entry->ino, &entry->inode) != 0)
            return -1;
        entry->dirty = 0;
    }

    if(info != NULL)
        set_fsinfo_stat(&info->stat, &entry->inode);
    return 0;
}

static int inode_cache_flush(ext3_t* ext3, ewokos_addr_t node, fsinfo_t* info) {
    inode_cache_t* entry = inode_cache_find_by_node(node);

    if(entry == NULL)
        return 0;
    return inode_cache_flush_entry(ext3, entry, info);
}

static int inode_cache_sync_node(ext3_t* ext3, ewokos_addr_t node, fsinfo_t* info) {
    inode_cache_t* entry = inode_cache_find_by_node(node);

    if(entry == NULL)
        return 0;

    set_inode_stat(&info->stat, &entry->inode);
    entry->dirty = 1;
    return inode_cache_flush_entry(ext3, entry, info);
}

/* Close always flushes before dropping the reference, so an over-drop
 * (e.g. an inherited fd closed before its FS_CMD_DUP arrived) can at
 * worst force a reload from the just-persisted inode, never data loss. */
static void inode_cache_drop(ewokos_addr_t node) {
    inode_cache_t** pp = &_inode_cache;

    while(*pp != NULL) {
        inode_cache_t* entry = *pp;
        if(entry->node == node) {
            if(--entry->refs <= 0) {
                *pp = entry->next;
                free(entry);
            }
            return;
        }
        pp = &entry->next;
    }
}

static uint32_t dir_block_count(ext3_t* ext3, const EXT3_INODE* inode) {
    uint32_t block_size = ext3_block_size(ext3);
    if(inode->i_size == 0)
        return 0;
    return (inode->i_size + block_size - 1) / block_size;
}

static int32_t dirent_name_equals(const EXT3_DIR_T* dp, const char* name) {
    size_t len = strlen(name);
    return dp->name_len == len && memcmp(dp->name, name, len) == 0;
}

static int32_t dirent_type_to_fs(const EXT3_DIR_T* dp, const EXT3_INODE* inode) {
    if(dp->file_type == EXT3_FT_DIR || (inode->i_mode & 0xF000) == EXT3_S_IFDIR)
        return FS_TYPE_DIR;
    if(dp->file_type == EXT3_FT_FILE || (inode->i_mode & 0xF000) == EXT3_S_IFREG)
        return FS_TYPE_FILE;
    return FS_TYPE_UNKNOWN;
}

static void set_fsinfo_stat(node_stat_t* stat, EXT3_INODE* inode) {
    stat->atime = inode->i_atime;
    stat->ctime = inode->i_ctime;
    stat->mtime = inode->i_mtime;
    stat->gid = inode->i_gid;
    stat->uid = inode->i_uid;
    stat->links_count = inode->i_links_count;
    stat->mode = inode->i_mode;
    stat->size = inode->i_size;
}

static void set_inode_stat(node_stat_t* stat, EXT3_INODE* inode) {
    inode->i_atime = stat->atime;
    inode->i_ctime = stat->ctime;
    inode->i_mtime = stat->mtime;
    inode->i_gid = stat->gid;
    inode->i_uid = stat->uid;
    inode->i_links_count = stat->links_count;
    //keep the on-disk file-type bits (S_IFDIR/S_IFREG): the VFS stat
    //mode only carries permissions, a plain chmod must not wipe them.
    inode->i_mode = (inode->i_mode & 0xF000) | (stat->mode & 0x0FFF);
    inode->i_size = stat->size;
}

#define NEW_NODES_BATCH 64

static int32_t add_nodes(ext3_t* ext3, EXT3_INODE *ip, fsinfo_t* dinfo) {
    char *cp;
    EXT3_DIR_T  *dp;
    uint32_t block_size = ext3_block_size(ext3);
    uint32_t blocks = dir_block_count(ext3, ip);
    char buf[EXT3_MAX_BLOCK_SIZE + 1];

    fsinfo_t* kids = NULL;
    uint32_t kid_num = 0;
    uint32_t kid_cap = 0;

    for(uint32_t lbk = 0; lbk < blocks; lbk++) {
        memset(buf, 0, sizeof(buf));
        int32_t rd = ext3_read_block(ext3, ip, buf, (int32_t)block_size, (int32_t)(lbk * block_size));
        if(rd <= 0)
            continue;
        dp = (EXT3_DIR_T *)buf;
        cp = buf;

        while (cp < (buf + block_size)){
            if(dp->name_len == 0 || dp->rec_len < 12 ||
                    dp->rec_len < (uint16_t)(4 * ((8 + dp->name_len + 3) / 4)) ||
                    (cp + dp->rec_len) > (buf + block_size))
                break;

            if(dp->inode != 0 && !dirent_name_equals(dp, ".") && !dirent_name_equals(dp, "..")) {
                int32_t ino = dp->inode;
                EXT3_INODE ip_node;
                if(ext3_node_by_ino(ext3, ino, &ip_node) == 0) {
                    int32_t type = dirent_type_to_fs(dp, &ip_node);
                    if(type == FS_TYPE_DIR || type == FS_TYPE_FILE) {
                        if(kid_num >= kid_cap) { //grow geometrically, one realloc per entry is O(n^2)
                            kid_cap = (kid_cap == 0) ? 16 : (kid_cap * 2);
                            kids = realloc(kids, sizeof(fsinfo_t) * kid_cap);
                        }
                        fsinfo_t* f = &kids[kid_num];
                        memset(f, 0, sizeof(fsinfo_t));
                        memcpy(f->name, dp->name, dp->name_len);
                        f->name[dp->name_len] = 0;
                        f->type = type;
                        f->data = (uint32_t)ino;
                        set_fsinfo_stat(&f->stat, &ip_node);
                        kid_num++;
                    }
                }
            }
            cp += dp->rec_len;
            dp = (EXT3_DIR_T *)cp;
        }
    }

    if(kid_num == 0)
        return 0;

    //only preload the first level under the mount root; deeper directories
    //are expanded lazily by vfsd through FS_CMD_KIDS.
    for(uint32_t off = 0; off < kid_num; off += NEW_NODES_BATCH) {
        uint32_t n = kid_num - off;
        if(n > NEW_NODES_BATCH)
            n = NEW_NODES_BATCH;
        if(vfs_new_nodes(&kids[off], n, dinfo->node) != 0) {
            //fall back to the one-by-one path (old vfsd)
            for(uint32_t j = 0; j < n; j++)
                vfs_new_node(&kids[off+j], dinfo->node, false, false);
        }
    }
    free(kids);
    return 0;
}

static int sdext_mount(vdevice_t* dev, fsinfo_t* info, void* p) {
    (void)dev;
    ext3_t* ext3 = (ext3_t*)p;
    EXT3_INODE root_node;
    info->state |= FS_STATE_KIDS_LOADED;
    if(ext3_node_by_fname(ext3, "/", &root_node) != 0)
        return -1;
    add_nodes(ext3, &root_node, info);
    return 0;
}

static int sdext_create(vdevice_t* dev, int pid, fsinfo_t* info_to, fsinfo_t* info, void* p) {
    (void)dev;
    ext3_t* ext3 = (ext3_t*)p;
    int32_t ino_to = (int32_t)info_to->data;
    if(ino_to == 0) ino_to = 2;

    EXT3_INODE inode_to;
    if(ext3_node_by_ino(ext3, ino_to, &inode_to) != 0)
        return -1;

    int ino = -1;
    if(FS_IS_TYPE(info->type, FS_TYPE_DIR))  {
        info->stat.size = ext3_block_size(ext3);
        ino = ext3_create_dir(ext3, ino_to, &inode_to, info->name, info->stat.uid, info->stat.gid, info->stat.mode);
    }
    else {
        info->stat.size = 0;
        ino = ext3_create_file(ext3, ino_to, &inode_to, info->name, info->stat.uid, info->stat.gid, info->stat.mode);
    }

    if(ino == -1)
        return -1;
    /* journal the new dirent + inode + bitmap updates now: a crash
     * before the next commit must not lose the just-created entry */
    if(ext3_commit(ext3) != 0)
        return -1;
    info->data = ino;
    if(FS_IS_TYPE(info->type, FS_TYPE_DIR))
        info->state |= FS_STATE_KIDS_LOADED;
    return 0;
}

static int sdext_open(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info, int oflag, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;

    /* Only files carry driver-side per-open inode state; every file open
     * takes one cache reference (released by the matching FS_CMD_CLOSE)
     * so a writer fd's cached inode stays alive while other fds of the
     * same file come and go. */
    if(!FS_IS_TYPE(info->type, FS_TYPE_FILE))
        return 0;

    ext3_t* ext3 = (ext3_t*)p;
    int32_t ino = (int32_t)info->data;
    if(ino == 0)
        return -1;

    EXT3_INODE inode;
    if(ext3_node_by_ino(ext3, ino, &inode) != 0) {
        return -1;
    }

    if((oflag & O_TRUNC) != 0) {
        if(ext3_truncate(ext3, (uint32_t)ino, &inode) != 0)
            return -1;
        /* the truncated state (freed blocks, zeroed size) must be
         * journaled before the file is handed out again */
        if(ext3_commit(ext3) != 0)
            return -1;
        set_fsinfo_stat(&info->stat, &inode);
    }
    if(inode_cache_seed(info->node, (uint32_t)ino, &inode, (oflag & O_TRUNC) != 0) == NULL)
        return -1;
    return 0;
}

static int sdext_set(vdevice_t* dev, int from_pid, fsinfo_t* info, void* p) {
    (void)dev;
    (void)from_pid;
    ext3_t* ext3 = (ext3_t*)p;
    int32_t ino = (int32_t)info->data;
    if(ino == 0)
        return -1;

    if(inode_cache_find_by_node(info->node) != NULL)
        return inode_cache_sync_node(ext3, info->node, info);

    EXT3_INODE inode;
    if(ext3_node_by_ino(ext3, ino, &inode) != 0) {
        return -1;
    }

    set_inode_stat(&info->stat, &inode);
    ext3_put_node(ext3, ino, &inode);
    (void)ext3_commit(ext3);
    return 0;
}

static int sdext_get(vdevice_t* dev, int from_pid, const char* fname, fsinfo_t* info, void* p) {
    (void)dev;
    ext3_t* ext3 = (ext3_t*)p;
    EXT3_DIR_T dirp;
    uint32_t ino = ext3_ino_by_fname(ext3, fname, &dirp);
    if(ino <= 0)
        return -1;

    EXT3_INODE inode;
    if(ext3_node_by_ino(ext3, ino, &inode) != 0)
        return -1;

    memset(info, 0, sizeof(fsinfo_t));
    strcpy(info->name, dirp.name);
    info->type = dirent_type_to_fs(&dirp, &inode);
    info->data = (uint32_t)ino;
    set_fsinfo_stat(&info->stat, &inode);
    return 0;
}

static fsinfo_t* sdext_kids(vdevice_t* dev, fsinfo_t* info_dir, uint32_t* num, void* p) {
    (void)dev;
    fsinfo_t* ret = NULL;
    *num = 0;
    if(!FS_IS_TYPE(info_dir->type, FS_TYPE_DIR))
        return NULL;

    ext3_t* ext3 = (ext3_t*)p;
    int32_t ino_dir = (int32_t)info_dir->data;
    if(ino_dir == 0) ino_dir = 2;

    EXT3_INODE inode_dir;
    if(ext3_node_by_ino(ext3, ino_dir, &inode_dir) != 0)
        return NULL;

    char *cp;
    EXT3_DIR_T  *dp;
    uint32_t block_size = ext3_block_size(ext3);
    uint32_t blocks = dir_block_count(ext3, &inode_dir);
    char buf[EXT3_MAX_BLOCK_SIZE + 1];

    for(uint32_t lbk = 0; lbk < blocks; lbk++) {
        memset(buf, 0, sizeof(buf));
        if(ext3_read_block(ext3, &inode_dir, buf, (int32_t)block_size, (int32_t)(lbk * block_size)) <= 0)
            continue;
        dp = (EXT3_DIR_T *)buf;
        cp = buf;

        while (cp < (buf + block_size)){
            if(dp->name_len == 0 || dp->rec_len < 12 ||
                    dp->rec_len < (uint16_t)(4 * ((8 + dp->name_len + 3) / 4)) ||
                    (cp + dp->rec_len) > (buf + block_size))
                break;

            if(dp->inode != 0 && !dirent_name_equals(dp, ".") && !dirent_name_equals(dp, "..")) {
                int32_t ino = dp->inode;
                EXT3_INODE ip_node;
                if(ext3_node_by_ino(ext3, ino, &ip_node) == 0) {
                    int32_t type = dirent_type_to_fs(dp, &ip_node);
                    if(type == FS_TYPE_DIR || type == FS_TYPE_FILE) {
                        fsinfo_t f;
                        memset(&f, 0, sizeof(fsinfo_t));
                        memcpy(f.name, dp->name, dp->name_len);
                        f.name[dp->name_len] = 0;
                        f.data = (uint32_t)ino;
                        f.type = type;
                        set_fsinfo_stat(&f.stat, &ip_node);

                        ret = realloc(ret, sizeof(fsinfo_t) * (*num + 1));
                        memcpy(&ret[*num], &f, sizeof(fsinfo_t));
                        (*num)++;
                    }
                }
            }
            cp += dp->rec_len;
            dp = (EXT3_DIR_T *)cp;
        }
    }
    return ret;
}

static int sdext_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
        void* buf, int size, int offset, void* p) {
    (void)dev;

    ext3_t* ext3 = (ext3_t*)p;
    int32_t ino = (int32_t)info->data;
    if(ino == 0)
        ino = 2;
    EXT3_INODE inode;
    inode_cache_t* entry = inode_cache_find_by_node(info->node);
    if(entry != NULL)
        memcpy(&inode, &entry->inode, sizeof(EXT3_INODE));
    else if(ext3_node_by_ino(ext3, ino, &inode) != 0) {
        return -1;
    }

    int rsize = info->stat.size - offset;
    if(rsize < size)
        size = rsize;
    if(size < 0)
        size = -1;

    if(size > 0) {
        size = ext3_read(ext3, &inode, buf, size, offset);
    }
    return size;
}

static int sdext_write(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
        const void* buf, int size, int offset, void* p) {
    (void)dev;

    ext3_t* ext3 = (ext3_t*)p;
    int32_t ino = (int32_t)info->data;
    if(ino == 0)
        return -1;

    inode_cache_t* entry = inode_cache_get(ext3, info->node, (uint32_t)ino);
    if(entry == NULL) {
        return -1;
    }
    /* data goes straight to the card (ordered mode); the inode and any
     * indirect/bitmap updates are journaled by the next commit, which
     * flush/close issue (a crash then only loses the tail of the file,
     * never corrupts the fs) */
    size = ext3_write(ext3, &entry->inode, buf, size, offset);
    if(size >= 0) {
        entry->dirty = 1;
        set_fsinfo_stat(&info->stat, &entry->inode);
    }
    return size;
}

static int sdext_flush(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    ext3_t* ext3 = (ext3_t*)p;
    if(inode_cache_flush(ext3, info->node, info) != 0)
        return -1;
    if(ext3_commit(ext3) != 0)
        return -1;
    return bsp_sd_flush();
}

static int sdext_close(vdevice_t* dev, int fd, int from_pid, ewokos_addr_t node, fsinfo_t* info, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    ext3_t* ext3 = (ext3_t*)p;
    if(inode_cache_flush(ext3, node, info) != 0)
        return -1;
    if(ext3_commit(ext3) != 0)
        return -1;
    inode_cache_drop(node);
    return bsp_sd_flush();
}

/* fork() inheritance: vfsd sends FS_CMD_DUP per inherited fd, and each
 * such fd sends its own FS_CMD_CLOSE later — take the matching cache
 * reference here so the shared inode entry outlives every holder. */
static int sdext_dup(vdevice_t* dev, int from_fd, int from_pid, int dup_fd, int dup_pid,
        ewokos_addr_t node, fsinfo_t* fsinfo, void* p) {
    (void)dev;
    (void)from_fd;
    (void)from_pid;
    (void)dup_fd;
    (void)dup_pid;
    (void)fsinfo;
    (void)p;
    inode_cache_t* entry = inode_cache_find_by_node(node);
    if(entry != NULL)
        entry->refs++;
    return 0;
}

static int sdext_unlink(vdevice_t* dev, fsinfo_t* info, const char* fname, void* p) {
    (void)dev;
    ext3_t* ext3 = (ext3_t*)p;
    int ret = FS_IS_TYPE(info->type, FS_TYPE_DIR) ? ext3_rmdir(ext3, fname) : ext3_unlink(ext3, fname);
    if(ret != 0)
        return -1;
    /* best effort: the node must go away in vfsd regardless; on ext3
     * the removal is already durable in the journal once this commit
     * succeeds, so a crash cannot resurrect the entry */
    (void)ext3_commit(ext3);
    return vfs_del_node(info->node);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    if((int16_t)getuid() >= 0) {
        klog("this process can only loaded by kernel!\n");
        return -1;
    }

    if(bsp_sd_init() != 0) {
        return -1;
    }

    /* classify the filesystem before mounting: ext3 is preferred and
     * checked first, plain ext2 is served in compatible mode */
    int32_t probed = ext3_probe(sd_read);
    if(probed == EXT3_PROBE_NONE) {
        klog("sdfsd: no ext2/ext3 filesystem found on sd card!\n");
        sd_quit();
        return -1;
    }

    ext3_t ext3;
    int32_t ret = ext3_init_ex2(&ext3, sd_read, sdext_sd_read_blocks, sd_write, sdext_sd_write_blocks,
            bsp_sd_flush, SD_BUFFER_SIZE);
    if(ret != 0) {
        if(ret == EXT3_ERR_JOURNAL)
            klog("sdfsd: ext3 journal unusable, run e2fsck!\n");
        sd_quit();
        return -1;
    }

    /*if(ext3_has_journal(&ext3)) {
        klog("ext3 (journal ino %u, %u txns / %u blocks recovered)\n",
                ext3.journal_ino, ext3.recovered_txns, ext3.recovered_blocks);
    }
    */

    vdevice_t dev;
    memset(&dev, 0, sizeof(vdevice_t));
    strcpy(dev.desc, ext3_has_journal(&ext3) ? "rootfs:ext3" : "rootfs:ext2");
    dev.mount = sdext_mount;
    dev.read = sdext_read;
    dev.write = sdext_write;
    dev.create = sdext_create;
    dev.open = sdext_open;
    dev.dup = sdext_dup;
    dev.close = sdext_close;
    dev.flush = sdext_flush;
    dev.set = sdext_set;
    dev.get = sdext_get;
    dev.kids = sdext_kids;
    dev.unlink = sdext_unlink;

    dev.extra_data = &ext3;
    device_run(&dev, "/", FS_TYPE_DIR, 0777);
    ext3_quit(&ext3);
    sd_quit();
    return 0;
}
