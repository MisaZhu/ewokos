#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ewoksys/wait.h>
#include <string.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <ewoksys/klog.h>
#include <stdio.h>
#include <time.h>

/*
 * Per-file backing store: data buffer PLUS the complete attribute set, so a
 * ramfs file carries the same information an ext3 inode does (mode/uid/gid/
 * links/atime/ctime/mtime/size) and stat()/ls -l report it exactly like a
 * standard on-disk file. The vfs seeds mode/uid/gid at create time
 * (vfs_create) and vfsd caches them; ramfs copies them into the inode here and
 * stays the authoritative source through ramfs_stat, keeping them in sync on
 * chmod/chown via ramfs_set. Directories hold no data buffer, so they keep
 * living in the vfs node cache and are reported through the ramfs_stat
 * fallback (their mode/uid/gid/times/links are stamped by ramfs_create).
 */
typedef struct {
    char* buf;
    size_t size;
    size_t capacity;
    uint16_t mode;         /* permission bits; S_IF* type bits are synthesized by libc from fsinfo.type */
    uint16_t uid;          /* owner */
    uint16_t gid;          /* group */
    uint16_t links_count;  /* 1 for a regular file, 2 for a directory */
    uint32_t atime;        /* last access (read) time, seconds since epoch */
    uint32_t ctime;        /* creation / inode-change time, seconds since epoch */
    uint32_t mtime;        /* last data modification time, seconds since epoch */
} ramfs_file_t;

static ramfs_file_t* ramfs_get_file(fsinfo_t* info) {
    return (ramfs_file_t*)(ewokos_addr_t)info->data;
}

/* Wall-clock seconds, matching ext3_now() in sdfsd so ramfs and the on-disk
 * rootfs stamp identical values (0 while the RTC has not been synced yet). */
static uint32_t ramfs_now(void) {
    time_t now = time(NULL);
    return (now <= 0) ? 0U : (uint32_t)now;
}

/* Conventional minimum link count: "." plus the parent dirent for a directory,
 * a single dirent for a regular file (matches ext3_mkdir/ext3_create). */
static uint16_t ramfs_default_links(uint32_t type) {
    return FS_IS_TYPE(type, FS_TYPE_DIR) ? 2 : 1;
}

static int ramfs_ensure_capacity(ramfs_file_t* file, size_t need) {
    size_t new_capacity;
    char* new_buf;

    if(file == NULL)
        return -1;
    if(need <= file->capacity)
        return 0;

    new_capacity = file->capacity > 0 ? file->capacity : 256;
    while(new_capacity < need) {
        size_t next = new_capacity << 1;

        if(next <= new_capacity) {
            new_capacity = need;
            break;
        }
        new_capacity = next;
    }

    new_buf = (char*)realloc(file->buf, new_capacity);
    if(new_buf == NULL)
        return -1;

    file->buf = new_buf;
    file->capacity = new_capacity;
    return 0;
}

/* Allocate an inode and seed the complete attribute set from the vfs-provided
 * fsinfo (vfs_create already filled mode/uid/gid). A fresh inode holds no data
 * yet: buf/size/capacity stay zero and grow on the first write. */
static ramfs_file_t* ramfs_inode_new(const fsinfo_t* info, uint32_t now) {
    ramfs_file_t* file = (ramfs_file_t*)malloc(sizeof(ramfs_file_t));

    if(file == NULL)
        return NULL;
    memset(file, 0, sizeof(*file));
    file->mode = info->stat.mode;
    file->uid = info->stat.uid;
    file->gid = info->stat.gid;
    file->links_count = info->stat.links_count != 0 ?
            info->stat.links_count : ramfs_default_links(info->type);
    file->ctime = info->stat.ctime != 0 ? info->stat.ctime : now;
    file->mtime = info->stat.mtime != 0 ? info->stat.mtime : now;
    file->atime = info->stat.atime != 0 ? info->stat.atime : file->ctime;
    return file;
}

/* Push the inode's authoritative attributes into the fsinfo stat so they ride
 * back to vfsd (create reply / write sync) and match what ramfs_stat reports. */
static void ramfs_inode_to_stat(const ramfs_file_t* file, fsinfo_t* info) {
    info->stat.size = (off_t)file->size;
    info->stat.mode = file->mode;
    info->stat.uid = file->uid;
    info->stat.gid = file->gid;
    info->stat.links_count = file->links_count;
    info->stat.atime = file->atime;
    info->stat.ctime = file->ctime;
    info->stat.mtime = file->mtime;
}

/* vfs_create fills uid/gid/mode but leaves the timestamps and links_count at
 * zero; sdfsd reads those from the ext3 inode, ramfs has to stamp them here or
 * ls -l shows a 1970 epoch date and a zero link count. Regular files also get
 * a driver-owned inode so ramfs is the authoritative source for every
 * attribute. The values ride back to vfsd on the create reply
 * (vfs_set_info/VFS_UPDATE_DATA). */
static int ramfs_create(vdevice_t* dev, int from_pid, fsinfo_t* info_to,
        fsinfo_t* info, void* p) {
    (void)dev;
    (void)from_pid;
    (void)info_to;
    (void)p;
    uint32_t now = ramfs_now();

    info->stat.atime = now;
    info->stat.ctime = now;
    info->stat.mtime = now;
    if(info->stat.links_count == 0)
        info->stat.links_count = ramfs_default_links(info->type);

    if(FS_IS_TYPE(info->type, FS_TYPE_FILE)) {
        ramfs_file_t* file = ramfs_inode_new(info, now);
        if(file == NULL)
            return -1;
        info->data = (ewokos_addr_t)file;
        ramfs_inode_to_stat(file, info);
    }
    return 0;
}

static int ramfs_open(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
        int oflag, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)p;
    /* vfsd clears the node stat.size on O_TRUNC, but the backing store is
     * ours: without discarding it here, a shorter rewrite leaves the old
     * tail behind and file->size pops back to the stale length on the next
     * write (ramfs_write only grows size). sdfsd does the same via
     * ext3_truncate in its open hook. Truncation is a data + inode change, so
     * mtime/ctime move too (mirroring ext3_truncate). */
    if((oflag & O_TRUNC) != 0) {
        ramfs_file_t* file = ramfs_get_file(info);
        uint32_t now = ramfs_now();
        if(file != NULL) {
            file->size = 0;
            file->mtime = now;
            file->ctime = now;
            ramfs_inode_to_stat(file, info);
        }
        else {
            info->stat.size = 0;
            info->stat.mtime = now;
            info->stat.ctime = now;
        }
    }
    return 0;
}

static int ramfs_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
        void* buf, int size, off_t offset, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)p;
    ramfs_file_t* file = ramfs_get_file(info);

    if(offset < 0)
        offset = 0;
    if(file == NULL || file->buf == NULL || file->size <= (size_t)offset)
        return 0;
    if((size_t)(size + offset) > file->size)
        size = (int)(file->size - (size_t)offset);
    if(size < 0)
        return 0;

    memcpy(buf, file->buf + offset, (size_t)size);
    /* a successful data read refreshes atime, mirroring ext3. */
    file->atime = ramfs_now();
    info->stat.atime = file->atime;
    return size;	
}

static int ramfs_write(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
        const void* buf, int size, off_t offset, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)p;
    ramfs_file_t* file = ramfs_get_file(info);
    size_t end;
    uint32_t now;

    if(offset < 0)
        offset = 0;
    if(size <= 0)
        return size;
    now = ramfs_now();
    if(file == NULL) {
        /* defensive: a node written before it was create-stamped (e.g. an
         * anonymous temp) still gets a full inode seeded from the fsinfo. */
        file = ramfs_inode_new(info, now);
        if(file == NULL)
            return -1;
        info->data = (ewokos_addr_t)file;
    }

    end = (size_t)offset + (size_t)size;
    if(ramfs_ensure_capacity(file, end) != 0)
        return -1;

    if((size_t)offset > file->size)
        memset(file->buf + file->size, 0, (size_t)offset - file->size);

    memcpy(file->buf + offset, buf, (size_t)size);
    if(end > file->size)
        file->size = end;
    /* a data write bumps mtime and ctime, mirroring ext3_write. */
    file->mtime = now;
    file->ctime = now;
    ramfs_inode_to_stat(file, info);
    return size;
}

static int ramfs_unlink(vdevice_t* dev, fsinfo_t* info, const char* fname, void* p) {
    (void)dev;
    (void)fname;
    (void)p;

    ramfs_file_t* file = ramfs_get_file(info);
    if(file != NULL) {
        if(file->buf != NULL)
            free(file->buf);
        free(file);
    }
    return vfs_del_node(info->node);
}

/* chmod/chown (and the post-write data writeback) land here after vfsd's
 * permission check. For VFS_UPDATE_METADATA the fsinfo carries the new
 * mode/uid/gid; for VFS_UPDATE_DATA vfsd already overwrote them with the
 * cached values, so the sync is a no-op. Either way the inode stays the
 * authoritative copy of the ownership and permission bits. Nodes we do not
 * back with an inode (directories) keep their metadata in the vfs cache. */
static int ramfs_set(vdevice_t* dev, int from_pid, fsinfo_t* info, void* p) {
    (void)dev;
    (void)from_pid;
    (void)p;
    ramfs_file_t* file = ramfs_get_file(info);

    if(file == NULL)
        return 0;
    file->mode = info->stat.mode;
    file->uid = info->stat.uid;
    file->gid = info->stat.gid;
    if(info->stat.links_count != 0)
        file->links_count = info->stat.links_count;
    return 0;
}

/* ls -l/stat reach the driver through dev_stat (FS_CMD_STAT). Reporting
 * straight from the inode keeps ramfs authoritative for every attribute,
 * exactly like sdfsd reports from its ext3 inode, and avoids vfsd's cached
 * stat lagging for a file still open (size/mtime are only pushed on 256KB
 * boundaries or at close) or after an O_TRUNC that was never written. Nodes
 * with no inode (empty directories) keep the vfs-cached values ramfs_create
 * stamped; the mode carries permission bits only, libc synthesizes the S_IF*
 * type bits from fsinfo.type. */
static int ramfs_stat(vdevice_t* dev, int from_pid, fsinfo_t* info,
        node_stat_t* stat, void* p) {
    (void)dev;
    (void)from_pid;
    (void)p;
    ramfs_file_t* file = ramfs_get_file(info);

    *stat = info->stat;
    if(file != NULL) {
        stat->size = (off_t)file->size;
        stat->mode = file->mode;
        stat->uid = file->uid;
        stat->gid = file->gid;
        stat->links_count = file->links_count;
        stat->atime = file->atime;
        stat->ctime = file->ctime;
        stat->mtime = file->mtime;
    }
    if(stat->links_count == 0)
        stat->links_count = ramfs_default_links(info->type);
    return 0;
}

int main(int argc, char** argv) {
    const char* mnt_point = argc > 1 ? argv[1]: "/tmp";

    vdevice_t dev;
    memset(&dev, 0, sizeof(vdevice_t));
    strcpy(dev.desc, "ramfs");
    dev.open = ramfs_open;
    dev.read = ramfs_read;
    dev.write = ramfs_write;
    dev.create = ramfs_create;
    dev.set = ramfs_set;
    dev.stat = ramfs_stat;
    dev.unlink = ramfs_unlink;
    
    device_run(&dev, mnt_point, FS_TYPE_DIR, 0777, false);
    return 0;
}
