#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
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
#include <sd/partition.h>
#include <ext3/ext3fs.h>
#include <stdio.h>
#include <bsp/bsp_sd.h>
#include <ewoksys/usbmsc.h>

#if defined(__has_include)
# if __has_include(<q8b_platform.h>)
#  include <q8b_platform.h>
#  define SDFSD_HAS_Q8B_BOOTFS 1
# endif
#endif

#define SD_BUFFER_SIZE (1024*1024*64) //64M buffer size

/*
 * Locking model (multi_task IPC: every FS request runs in its own
 * kernel-spawned worker thread):
 *
 *   _fs_lock (rwlock) guards all shared fs state: the ext3 handle
 *   (dirty-block cache, journal, bitmaps), the inode cache list and
 *   the sd sector cache reached through it.
 *     - read-only requests (read/get/kids/mount) take it SHARED so
 *       concurrent readers run in parallel;
 *     - every mutating request (write/create/open/set/flush/close/
 *       dup/unlink) takes it EXCLUSIVE, so no two writes can ever
 *       interleave.
 *   The sd layer serializes its own device IO on top of this.
 *
 *   Synchronous IPC back into vfsd (vfs_new_nodes/vfs_del_node) must
 *   happen OUTSIDE the lock: vfsd serves devices in single-task mode
 *   and would otherwise deadlock waiting on a locked-out sdfsd worker.
 */
static pthread_rwlock_t _fs_lock = {0};

static inline void fs_rdlock(void) {
    pthread_rwlock_rdlock(&_fs_lock);
}

static inline void fs_wrlock(void) {
    pthread_rwlock_wrlock(&_fs_lock);
}

static inline void fs_unlock(void) {
    pthread_rwlock_unlock(&_fs_lock);
}


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

/* The same ext3 server is used for the on-board SD card and for a USB MSC
 * disk selected by init.  USB MSC transports 512-byte sectors; the ext3
 * library's block size is changed through sd_set_block_size() while it
 * probes the superblock, so the callbacks below derive their sector count
 * from sd_get_block_size() on every request. */
static int _usb_mode = 0;
static int _usb_dev_pid = -1;
static uint32_t _usb_partition_start = 0;
static int _boot_mode = 0;

#ifdef SDFSD_HAS_Q8B_BOOTFS
#define Q8B_BOOTFS_VA 0x60000000ULL
static uint8_t* _bootfs_va;
static uint32_t _bootfs_size;
static uint32_t _bootfs_block_size = 1024;

static int bootfs_map(void) {
    sys_info_t info;
    const q8b_platform_data_t* pd;
    if (sys_get_sys_info(&info) != 0)
        return -1;
    pd = (const q8b_platform_data_t*)info.platform_data;
    if (pd == NULL || pd->rd.phy_base == 0 || pd->rd.size == 0 ||
            (pd->rd.flags & Q8B_RD_FLAG_BOOT_ONLY) == 0)
        return -1;
    ewokos_addr_t ret = syscall3(SYS_MEM_MAP, (ewokos_addr_t)Q8B_BOOTFS_VA,
            (ewokos_addr_t)pd->rd.phy_base, (ewokos_addr_t)pd->rd.size);
    if (ret != (ewokos_addr_t)Q8B_BOOTFS_VA)
        return -1;
    _bootfs_va = (uint8_t*)(uintptr_t)Q8B_BOOTFS_VA;
    _bootfs_size = pd->rd.size;
    _bootfs_block_size = 1024;
    return 0;
}

static void bootfs_unmap(void) {
    if (_bootfs_va != NULL && _bootfs_size != 0)
        (void)syscall3(SYS_MEM_UNMAP, (ewokos_addr_t)Q8B_BOOTFS_VA,
                (ewokos_addr_t)_bootfs_size, 0);
    _bootfs_va = NULL;
    _bootfs_size = 0;
    _bootfs_block_size = 1024;
}

static int32_t bootfs_read_block(int32_t block, void* buf) {
    uint64_t off;
    uint32_t bytes = _bootfs_block_size;
    if (block < 0 || buf == NULL || bytes == 0 || _bootfs_va == NULL)
        return -1;
    off = (uint64_t)(uint32_t)block * bytes;
    if (off + bytes < off || off + bytes > _bootfs_size)
        return -1;
    memcpy(buf, _bootfs_va + off, bytes);
    if (block == 1 && bytes == 1024) {
        const uint8_t* sb = (const uint8_t*)buf;
        uint32_t log = (uint32_t)sb[24];
        if (log <= 2)
            _bootfs_block_size = 1024U << log;
    }
    return 0;
}

static int32_t bootfs_read_blocks(int32_t block, void* buf, uint32_t count) {
    uint32_t bytes = _bootfs_block_size;
    if (count == 0 || bytes == 0)
        return 0;
    for (uint32_t i = 0; i < count; i++) {
        if (bootfs_read_block(block + (int32_t)i,
                (uint8_t*)buf + i * bytes) != 0)
            return -1;
    }
    return 0;
}

static int32_t bootfs_write_block(int32_t block, const void* buf) {
    (void)block;
    (void)buf;
    return -1;
}

static int32_t bootfs_write_blocks(int32_t block, const void* buf,
        uint32_t count) {
    (void)block;
    (void)buf;
    (void)count;
    return -1;
}
#else
static void bootfs_unmap(void) {
}
#endif

static int usb_msc_io(int write, int32_t sector, void* buf, uint32_t count) {
    proto_t in, out;
    int ret = -1;

    if (count == 0 || count > USBMSC_MAX_SECTORS || sector < 0)
        return -1;
    PF->init(&in)->addi(&in,
            (ewokos_addr_t)((uint32_t)sector + _usb_partition_start))->addi(&in, count);
    if (write)
        PF->add(&in, buf, count * SECTOR_SIZE);
    PF->init(&out);
    if (dev_cntl_by_pid(_usb_dev_pid, write ? USBMSC_CMD_WRITE : USBMSC_CMD_READ,
            &in, &out) == 0) {
        if (write) {
            ret = 0;
        } else {
            int32_t size = 0;
            void* data = proto_read(&out, &size);
            if (data != NULL && size == (int32_t)(count * SECTOR_SIZE)) {
                memcpy(buf, data, (size_t)size);
                ret = 0;
            }
        }
    }
    PF->clear(&in);
    PF->clear(&out);
    return ret;
}

static int32_t usb_ext_read_block(int32_t block, void* buf) {
    uint32_t block_size = sd_get_block_size();
    uint32_t sectors = block_size / SECTOR_SIZE;
    uint8_t* p = (uint8_t*)buf;
    if (block < 0 || sectors == 0 || block_size % SECTOR_SIZE != 0)
        return -1;
    /* Some MSC bridges stall a READ10 data phase larger than one sector.
     * Keep ext3 block semantics while issuing the transport as single-sector
     * requests; this also keeps journal recovery compatible with those disks. */
    for (uint32_t i = 0; i < sectors; i++) {
        if (usb_msc_io(0, block * (int32_t)sectors + (int32_t)i,
                p + i * SECTOR_SIZE, 1) != 0)
            return -1;
    }
    return 0;
}

static int32_t usb_ext_write_block(int32_t block, const void* buf) {
    uint32_t block_size = sd_get_block_size();
    uint32_t sectors = block_size / SECTOR_SIZE;
    const uint8_t* p = (const uint8_t*)buf;
    if (block < 0 || sectors == 0 || block_size % SECTOR_SIZE != 0)
        return -1;
    for (uint32_t i = 0; i < sectors; i++) {
        if (usb_msc_io(1, block * (int32_t)sectors + (int32_t)i,
                (void*)(p + i * SECTOR_SIZE), 1) != 0)
            return -1;
    }
    return 0;
}

static int32_t usb_ext_read_blocks(int32_t block, void* buf, uint32_t count) {
    uint32_t block_size = sd_get_block_size();
    uint32_t sectors = block_size / SECTOR_SIZE;
    uint8_t* p = (uint8_t*)buf;
    if (block < 0 || sectors == 0 || block_size % SECTOR_SIZE != 0)
        return -1;
    while (count != 0) {
        uint32_t blocks = count;
        if (blocks * sectors > USBMSC_MAX_SECTORS)
            blocks = USBMSC_MAX_SECTORS / sectors;
        if (blocks == 0)
            return -1;
        for (uint32_t i = 0; i < blocks; i++) {
            if (usb_ext_read_block(block + (int32_t)i,
                    p + i * block_size) != 0)
                return -1;
        }
        block += (int32_t)blocks;
        p += blocks * block_size;
        count -= blocks;
    }
    return 0;
}

static int32_t usb_ext_write_blocks(int32_t block, const void* buf, uint32_t count) {
    uint32_t block_size = sd_get_block_size();
    uint32_t sectors = block_size / SECTOR_SIZE;
    const uint8_t* p = (const uint8_t*)buf;
    if (block < 0 || sectors == 0 || block_size % SECTOR_SIZE != 0)
        return -1;
    while (count != 0) {
        uint32_t blocks = count;
        if (blocks * sectors > USBMSC_MAX_SECTORS)
            blocks = USBMSC_MAX_SECTORS / sectors;
        if (blocks == 0)
            return -1;
        for (uint32_t i = 0; i < blocks; i++) {
            if (usb_ext_write_block(block + (int32_t)i,
                    p + i * block_size) != 0)
                return -1;
        }
        block += (int32_t)blocks;
        p += blocks * block_size;
        count -= blocks;
    }
    return 0;
}

static int32_t usb_read_sector(int32_t sector, void* buf) {
    return usb_msc_io(0, sector, buf, 1) == 0 ? SECTOR_SIZE : -1;
}

static int32_t rootfs_read_block(int32_t block, void* buf) {
#ifdef SDFSD_HAS_Q8B_BOOTFS
    if (_boot_mode)
        return bootfs_read_block(block, buf);
#endif
    return _usb_mode ? usb_ext_read_block(block, buf) : sd_read(block, buf);
}

static int32_t rootfs_read_blocks(int32_t block, void* buf, uint32_t count) {
#ifdef SDFSD_HAS_Q8B_BOOTFS
    if (_boot_mode)
        return bootfs_read_blocks(block, buf, count);
#endif
    return _usb_mode ? usb_ext_read_blocks(block, buf, count) : sd_read_blocks(block, buf, count);
}

static int32_t rootfs_write_block(int32_t block, const void* buf) {
#ifdef SDFSD_HAS_Q8B_BOOTFS
    if (_boot_mode)
        return bootfs_write_block(block, buf);
#endif
    return _usb_mode ? usb_ext_write_block(block, buf) : sd_write(block, buf);
}

static int32_t rootfs_write_blocks(int32_t block, const void* buf, uint32_t count) {
#ifdef SDFSD_HAS_Q8B_BOOTFS
    if (_boot_mode)
        return bootfs_write_blocks(block, buf, count);
#endif
    return _usb_mode ? usb_ext_write_blocks(block, buf, count) : sd_write_blocks(block, buf, count);
}

static int32_t rootfs_flush(void) {
#ifdef SDFSD_HAS_Q8B_BOOTFS
    if (_boot_mode)
        return 0;
#endif
    if (!_usb_mode)
        return sd_flush();
    proto_t in, out;
    PF->init(&in);
    PF->init(&out);
    int ret = dev_cntl_by_pid(_usb_dev_pid, USBMSC_CMD_FLUSH, &in, &out);
    PF->clear(&in);
    PF->clear(&out);
    return ret;
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
    stat->size = ext3_inode_file_size(inode);
}

static void set_inode_stat(node_stat_t* stat, EXT3_INODE* inode) {
    inode->i_atime = stat->atime;
    inode->i_ctime = stat->ctime;
    inode->i_mtime = stat->mtime;
    inode->i_gid = stat->gid;
    inode->i_uid = stat->uid;
    /* i_links_count is owned by the fs (create/link/unlink/mkdir):
     * a generic stat update (chmod/chown) arrives with the VFS's
     * links_count, which vfsd never maintains (0) and was wiping the
     * real on-disk value (fsck: "deleted inode has zero dtime"). */
    //keep the on-disk file-type bits (S_IFDIR/S_IFREG): the VFS stat
    //mode only carries permissions, a plain chmod must not wipe them.
    inode->i_mode = (inode->i_mode & 0xF000) | (stat->mode & 0x0FFF);
    ext3_inode_set_file_size(inode, stat->size);
}

#define NEW_NODES_BATCH 64

/* Scan one directory's data blocks and collect its child entries.
 * Touches only the ext3 read path, safe under the shared fs lock. */
static fsinfo_t* dir_collect_entries(ext3_t* ext3, EXT3_INODE* dir_inode, uint32_t* num) {
    char *cp;
    EXT3_DIR_T  *dp;
    uint32_t block_size = ext3_block_size(ext3);
    uint32_t blocks = dir_block_count(ext3, dir_inode);
    char buf[EXT3_MAX_BLOCK_SIZE + 1];

    fsinfo_t* kids = NULL;
    uint32_t kid_num = 0;
    uint32_t kid_cap = 0;

    for(uint32_t lbk = 0; lbk < blocks; lbk++) {
        memset(buf, 0, sizeof(buf));
        int32_t rd = ext3_read_block(ext3, dir_inode, buf, (int32_t)block_size, (int32_t)(lbk * block_size));
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

    *num = kid_num;
    return kids;
}

/* Register collected children with vfsd. Issues synchronous IPC back
 * into vfsd: must run OUTSIDE _fs_lock (see locking model above). */
static void dir_publish_entries(fsinfo_t* kids, uint32_t num, ewokos_addr_t parent) {
    //only preload the first level under the mount root; deeper directories
    //are expanded lazily by vfsd through FS_CMD_KIDS.
    for(uint32_t off = 0; off < num; off += NEW_NODES_BATCH) {
        uint32_t n = num - off;
        if(n > NEW_NODES_BATCH)
            n = NEW_NODES_BATCH;
        if(vfs_new_nodes(&kids[off], n, parent) != 0) {
            //fall back to the one-by-one path (old vfsd)
            for(uint32_t j = 0; j < n; j++)
                vfs_new_node(&kids[off+j], parent, false, false);
        }
    }
}

static int sdext_mount(vdevice_t* dev, fsinfo_t* info, void* p) {
    (void)dev;
    ext3_t* ext3 = (ext3_t*)p;
    EXT3_INODE root_node;
    fsinfo_t* kids = NULL;
    uint32_t num = 0;

    fs_rdlock();
    info->state |= FS_STATE_KIDS_LOADED;
    int ret = ext3_node_by_fname(ext3, "/", &root_node);
    if(ret == 0)
        kids = dir_collect_entries(ext3, &root_node, &num);
    fs_unlock();

    if(ret != 0)
        return -1;

    dir_publish_entries(kids, num, info->node);
    free(kids);
    return 0;
}

static int sdext_create(vdevice_t* dev, int pid, fsinfo_t* info_to, fsinfo_t* info, void* p) {
    (void)dev;
    ext3_t* ext3 = (ext3_t*)p;
    int32_t ino_to = (int32_t)info_to->data;
    if(ino_to == 0) ino_to = 2;

    fs_wrlock();
    EXT3_INODE inode_to;
    if(ext3_node_by_ino(ext3, ino_to, &inode_to) != 0) {
        fs_unlock();
        return -1;
    }

    int ino = -1;
    if(FS_IS_TYPE(info->type, FS_TYPE_DIR))  {
        info->stat.size = ext3_block_size(ext3);
        ino = ext3_create_dir(ext3, ino_to, &inode_to, info->name, info->stat.uid, info->stat.gid, info->stat.mode);
    }
    else {
        info->stat.size = 0;
        ino = ext3_create_file(ext3, ino_to, &inode_to, info->name, info->stat.uid, info->stat.gid, info->stat.mode);
    }

    if(ino == -1) {
        fs_unlock();
        return -1;
    }
    /* journal the new dirent + inode + bitmap updates now: a crash
     * before the next commit must not lose the just-created entry */
    int ret = ext3_commit(ext3);
    fs_unlock();

    if(ret != 0)
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

    if(ext3_is_readonly(ext3) &&
            ((oflag & (O_WRONLY | O_RDWR | O_TRUNC)) != 0))
        return -1;

    int32_t ino = (int32_t)info->data;
    if(ino == 0)
        return -1;

    fs_wrlock();
    EXT3_INODE inode;
    if(ext3_node_by_ino(ext3, ino, &inode) != 0) {
        fs_unlock();
        return -1;
    }

    if((oflag & O_TRUNC) != 0) {
        if(ext3_truncate(ext3, (uint32_t)ino, &inode) != 0) {
            fs_unlock();
            return -1;
        }
        /* the truncated state (freed blocks, zeroed size) must be
         * journaled before the file is handed out again */
        if(ext3_commit(ext3) != 0) {
            fs_unlock();
            return -1;
        }
        set_fsinfo_stat(&info->stat, &inode);
    }
    inode_cache_t* entry = inode_cache_seed(info->node, (uint32_t)ino, &inode, (oflag & O_TRUNC) != 0);
    fs_unlock();

    if(entry == NULL)
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

    fs_wrlock();
    if(inode_cache_find_by_node(info->node) != NULL) {
        int ret = inode_cache_sync_node(ext3, info->node, info);
        fs_unlock();
        return ret;
    }

    EXT3_INODE inode;
    if(ext3_node_by_ino(ext3, ino, &inode) != 0) {
        fs_unlock();
        return -1;
    }

    set_inode_stat(&info->stat, &inode);
    ext3_put_node(ext3, ino, &inode);
    (void)ext3_commit(ext3);
    fs_unlock();
    return 0;
}

static int sdext_get(vdevice_t* dev, int from_pid, const char* fname, fsinfo_t* info, void* p) {
    (void)dev;
    ext3_t* ext3 = (ext3_t*)p;

    fs_rdlock();
    EXT3_DIR_T dirp;
    uint32_t ino = ext3_ino_by_fname(ext3, fname, &dirp);
    if(ino <= 0) {
        fs_unlock();
        return -1;
    }

    EXT3_INODE inode;
    if(ext3_node_by_ino(ext3, ino, &inode) != 0) {
        fs_unlock();
        return -1;
    }

    memset(info, 0, sizeof(fsinfo_t));
    strcpy(info->name, dirp.name);
    info->type = dirent_type_to_fs(&dirp, &inode);
    info->data = (uint32_t)ino;
    set_fsinfo_stat(&info->stat, &inode);
    fs_unlock();
    return 0;
}

static fsinfo_t* sdext_kids(vdevice_t* dev, fsinfo_t* info_dir, uint32_t* num, void* p) {
    (void)dev;
    *num = 0;
    if(!FS_IS_TYPE(info_dir->type, FS_TYPE_DIR))
        return NULL;

    ext3_t* ext3 = (ext3_t*)p;
    int32_t ino_dir = (int32_t)info_dir->data;
    if(ino_dir == 0) ino_dir = 2;

    fs_rdlock();
    EXT3_INODE inode_dir;
    if(ext3_node_by_ino(ext3, ino_dir, &inode_dir) != 0) {
        fs_unlock();
        return NULL;
    }

    fsinfo_t* ret = dir_collect_entries(ext3, &inode_dir, num);
    fs_unlock();
    return ret;
}

static int sdext_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
        void* buf, int size, off_t offset, void* p) {
    (void)dev;

    ext3_t* ext3 = (ext3_t*)p;
    int32_t ino = (int32_t)info->data;
    if(ino == 0)
        ino = 2;

    fs_rdlock();
    EXT3_INODE inode;
    inode_cache_t* entry = inode_cache_find_by_node(info->node);
    if(entry != NULL)
        memcpy(&inode, &entry->inode, sizeof(EXT3_INODE));
    else if(ext3_node_by_ino(ext3, ino, &inode) != 0) {
        fs_unlock();
        return -1;
    }

    off_t rsize = (off_t)info->stat.size - offset;
    if(rsize < 0)
        rsize = 0; /* reads at/ past EOF are end-of-file (0), not an error */
    if(rsize < size)
        size = (int)rsize;
    if(size < 0)
        size = -1;

    if(size > 0) {
        size = ext3_read(ext3, &inode, buf, size, offset);
    }
    fs_unlock();
    return size;
}

static int sdext_write(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
        const void* buf, int size, off_t offset, void* p) {
    (void)dev;

    ext3_t* ext3 = (ext3_t*)p;
    int32_t ino = (int32_t)info->data;
    if(ino == 0)
        return -1;

    fs_wrlock();
    inode_cache_t* entry = inode_cache_get(ext3, info->node, (uint32_t)ino);
    if(entry == NULL) {
        fs_unlock();
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
    fs_unlock();
    return size;
}

static int sdext_flush(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    ext3_t* ext3 = (ext3_t*)p;

    fs_wrlock();
    int ret = inode_cache_flush(ext3, info->node, info);
    if(ret == 0)
        ret = ext3_commit(ext3);
    fs_unlock();

    if(ret != 0)
        return -1;
    return rootfs_flush();
}

static int sdext_close(vdevice_t* dev, int fd, int from_pid, ewokos_addr_t node, fsinfo_t* info, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    ext3_t* ext3 = (ext3_t*)p;

    fs_wrlock();
    int ret = inode_cache_flush(ext3, node, info);
    if(ret == 0)
        ret = ext3_commit(ext3);
    if(ret == 0)
        inode_cache_drop(node);
    fs_unlock();

    if(ret != 0)
        return -1;
    return rootfs_flush();
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

    fs_wrlock();
    inode_cache_t* entry = inode_cache_find_by_node(node);
    if(entry != NULL)
        entry->refs++;
    fs_unlock();
    return 0;
}

static int sdext_unlink(vdevice_t* dev, fsinfo_t* info, const char* fname, void* p) {
    (void)dev;
    ext3_t* ext3 = (ext3_t*)p;

    fs_wrlock();
    int ret = FS_IS_TYPE(info->type, FS_TYPE_DIR) ? ext3_rmdir(ext3, fname) : ext3_unlink(ext3, fname);
    if(ret == 0) {
        /* best effort: the node must go away in vfsd regardless; on ext3
         * the removal is already durable in the journal once this commit
         * succeeds, so a crash cannot resurrect the entry */
        (void)ext3_commit(ext3);
    }
    fs_unlock();

    if(ret != 0)
        return -1;
    /* synchronous IPC back into vfsd: outside the lock */
    return vfs_del_node(info->node);
}

static int usb_wait_ready(void) {
    /* A Type-C MSC disk can need several port scans after VBUS and the
     * bridge's media have settled.  usbhostd keeps retrying failed MSC
     * enumeration in the background; keep the rootfs probe alive while that
     * retry reaches the ready state. */
    for (int i = 0; i < 120; i++) {
        proto_t in, out;
        PF->init(&in);
        PF->init(&out);
        int ret = dev_cntl_by_pid(_usb_dev_pid, USBMSC_CMD_INFO, &in, &out);
        PF->clear(&in);
        int ready = ret == 0 && proto_read_int(&out) == 1;
        PF->clear(&out);
        if (ready)
            return 0;
        usleep(100000);
    }
    return -1;
}

int main(int argc, char** argv) {
    const char* mount_point = "/";
    int sd_mode = 1;
    if((int16_t)getuid() > 0) {
        klog("this process can only loaded by root!\n");
        return -1;
    }

    if(argc >= 2 && strcmp(argv[1], "-b") == 0) {
#ifdef SDFSD_HAS_Q8B_BOOTFS
        _boot_mode = 1;
        sd_mode = 0;
        mount_point = argc >= 3 ? argv[2] : "/boot";
        if (bootfs_map() != 0) {
            klog("sdfsd: boot initramfs unavailable!\n");
            return -1;
        }
#else
        klog("sdfsd: boot initramfs unsupported!\n");
        return -1;
#endif
    } else if(argc >= 2 && strcmp(argv[1], "-u") == 0) {
        const char* dev_path;
        fsinfo_t dev_info;
        sd_mode = 0;
        dev_path = argc >= 3 ? argv[2] : "/dev/usb0";
        mount_point = argc >= 4 ? argv[3] : "/";
        if(vfs_get_by_name(dev_path, &dev_info) != 0) {
            klog("sdfsd: usb dev %s not found!\n", dev_path);
            return -1;
        }
        _usb_dev_pid = dev_info.mount_pid;
        if(usb_wait_ready() != 0) {
            klog("sdfsd: usb device not ready!\n");
            return -1;
        }
        _usb_mode = 1;
        _usb_partition_start = get_rootfs_entry(usb_read_sector);
        klog("sdfsd: usb rootfs partition lba=%u\n", _usb_partition_start);
    } else if(bsp_sd_init() != 0) {
        return -1;
    }

    /* classify the filesystem before mounting: ext3 is preferred and
     * checked first, plain ext2 is served in compatible mode */
    /* ext3_probe() is tied to the block-device helper and performs a device
     * probe before reading the superblock.  The boot image is already a
     * complete filesystem image in RAM, so initialize it directly instead
     * of probing the absent SD device. */
    int32_t probed = _boot_mode ? EXT3_PROBE_EXT3 : ext3_probe(rootfs_read_block);
    if(probed == EXT3_PROBE_NONE) {
        klog("sdfsd: no ext2/ext3 filesystem found on %s!\n",
                _boot_mode ? "boot initramfs" : (sd_mode ? "sd card" : "usb disk"));
        if(_boot_mode)
            bootfs_unmap();
        else if(sd_mode)
            sd_quit();
        return -1;
    }

    ext3_t ext3;
    /* Mount read-write; a damaged journal falls back to a read-only mount
     * below instead of being treated as an ext2 volume. */
    int readonly = _boot_mode;
    int32_t ret = _boot_mode ?
            ext3_init_ro_ex2(&ext3, rootfs_read_block, rootfs_read_blocks,
                    rootfs_write_block, rootfs_write_blocks, NULL, SD_BUFFER_SIZE) :
            ext3_init_ex2(&ext3, rootfs_read_block, rootfs_read_blocks,
                    rootfs_write_block, rootfs_write_blocks, rootfs_flush, SD_BUFFER_SIZE);
    if(ret != 0) {
        if(ret == EXT3_ERR_JOURNAL) {
            klog("sdfsd: ext3 journal unusable (jbd=%d), run e2fsck!\n",
                    ext3.journal_error);
            /* A damaged journal must never be treated as an ext2 volume:
             * doing so can overwrite metadata while recovery is pending.
             * Keep the rootfs available read-only so init/login and the
             * raw-SD repair tools can still be used to repair it offline. */
            readonly = 1;
            ret = ext3_init_ro_ex2(&ext3, rootfs_read_block, rootfs_read_blocks,
                    rootfs_write_block, rootfs_write_blocks, NULL, SD_BUFFER_SIZE);
            if(ret == 0)
                klog("sdfsd: mounted read-only pending journal repair\n");
        }
        if(ret != 0) {
            if(_boot_mode)
                bootfs_unmap();
            else if(sd_mode)
                sd_quit();
            return -1;
        }
    }

    /*if(ext3_has_journal(&ext3)) {
        klog("ext3 (journal ino %u, %u txns / %u blocks recovered)\n",
                ext3.journal_ino, ext3.recovered_txns, ext3.recovered_blocks);
    }
    */

    vdevice_t dev;
    memset(&dev, 0, sizeof(vdevice_t));
    strcpy(dev.desc, _boot_mode ? "bootfs:ext3" :
            (ext3_has_journal(&ext3) ? "rootfs:ext3" : "rootfs:ext2"));
    dev.mount = sdext_mount;
    dev.read = sdext_read;
    dev.write = readonly ? NULL : sdext_write;
    dev.create = readonly ? NULL : sdext_create;
    dev.open = sdext_open;
    dev.dup = sdext_dup;
    dev.close = sdext_close;
    dev.flush = readonly ? NULL : sdext_flush;
    dev.set = readonly ? NULL : sdext_set;
    dev.get = sdext_get;
    dev.kids = sdext_kids;
    dev.unlink = readonly ? NULL : sdext_unlink;

    dev.extra_data = &ext3;

    /* init the fs lock while still single-threaded, before any IPC
     * worker can exist; multi_task serves each request in its own
     * kernel-spawned thread, the rwlock keeps writes exclusive */
    pthread_rwlock_init(&_fs_lock, NULL);
    device_run(&dev, mount_point, FS_TYPE_DIR, 0777, true);
    ext3_quit(&ext3);
    if(_boot_mode)
        bootfs_unmap();
    else if(sd_mode)
        sd_quit();
    return 0;
}
