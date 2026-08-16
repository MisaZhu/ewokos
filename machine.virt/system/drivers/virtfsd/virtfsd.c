#include <stdlib.h>
#include <unistd.h>
#include <ewoksys/wait.h>
#include <string.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <ewoksys/klog.h>
#include <ewoksys/mmio.h>
#include <sysinfo.h>
#include <fcntl.h>
#include <stdio.h>
#include <arch/virt/virtfs.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define VIRTFS_DEBUG 0
#if VIRTFS_DEBUG
#define FS_DBG(fmt, ...) klog(fmt, ##__VA_ARGS__)
#else
#define FS_DBG(fmt, ...) \
    do                   \
    {                    \
    } while (0)
#endif

typedef struct{
    uint32_t fid;
    uint32_t pfid;
    uint8_t flag;
}virtfs_node_t;

static virtfs_node_t *nodes = NULL;
static uint32_t virtfs_node_count = 0;
static uint32_t virtfs_node_used = 0;

#define NEW_NODES_BATCH 64

static int alloc_node(uint32_t pfid)
{
    if(virtfs_node_count - virtfs_node_used < 1024){
        virtfs_node_count += 1024;
        virtfs_node_t* new_nodes =
            (virtfs_node_t *)realloc(nodes, sizeof(virtfs_node_t) * virtfs_node_count);
        if(new_nodes == NULL)
            return -1;
        nodes = new_nodes;
    }
    nodes[virtfs_node_used].fid = virtfs_node_used;
    nodes[virtfs_node_used].pfid = pfid;
    nodes[virtfs_node_used].flag = 0;
    return virtfs_node_used++;
}

static int virtfs_ensure_open(virtfs_t fs, uint32_t fid, int oflag)
{
    if(fid >= virtfs_node_used)
        return -1;
    if(nodes[fid].flag)
        return 0;
    int ret = virtfs_open(fs, 1, fid, oflag & 0xFF);
    if(ret == 0)
        nodes[fid].flag = 1;
    return ret;
}

static int virtfs_dirent_is_dots(const struct virtfs_dir_entry *entry)
{
    if(entry->nlen == 1 && entry->name[0] == '.')
        return 1;
    if(entry->nlen == 2 && entry->name[0] == '.' && entry->name[1] == '.')
        return 1;
    return 0;
}

static int virtfs_fill_info(fsinfo_t *info, const struct virtfs_dir_entry *entry,
        uint32_t fid, virtfs_t fs)
{
    virtfs_stat_t stat;
    int nlen;

    if(entry->type == VIRTFS_TYPE_DIR)
        info->type = FS_TYPE_DIR;
    else if(entry->type == VIRTFS_TYPE_FILE)
        info->type = FS_TYPE_FILE;
    else
        return -1;

    if(virtfs_stat(fs, 1, fid, &stat) != 0)
        return -1;

    memset(info, 0, sizeof(fsinfo_t));
    nlen = MIN(entry->nlen, FS_NODE_NAME_MAX - 1);
    memcpy(info->name, entry->name, nlen);
    info->name[nlen] = '\0';
    info->type = (entry->type == VIRTFS_TYPE_DIR) ? FS_TYPE_DIR : FS_TYPE_FILE;
    info->data = fid;
    info->stat.size = stat.length;
    info->stat.mode = stat.mode | 0444;
    info->stat.atime = stat.atime;
    info->stat.mtime = stat.mtime;
    info->stat.gid = 100;
    info->stat.uid = 100;
    info->stat.mtime = stat.mtime;
    return 0;
}

static void debug_entry(struct virtfs_dir_entry *entry)
{
    FS_DBG("type:%d name:", entry->type, entry->nlen);
    for (int i = 0; i < entry->nlen; i++)
    {
        FS_DBG("%c", entry->name[i]);
    }
    FS_DBG("\n");
}

static fsinfo_t* virtfsd_list_directory(virtfs_t fs, uint32_t fid, uint32_t* num)
{
    char dir[1024];
    char name[256];
    fsinfo_t* infos = NULL;
    uint32_t count = 0;

    *num = 0;
    memset(dir, 0, sizeof(dir));
    if(virtfs_ensure_open(fs, fid, O_RDONLY) != 0)
        return NULL;

    uint64_t offset = 0;
    while(true) {
        int ret = virtfs_readdir(fs, 1, fid, dir, offset, sizeof(dir));
        if(ret <= 0)
            break;

        for(int i = 0; i < ret;) {
            struct virtfs_dir_entry *entry = (struct virtfs_dir_entry *)&dir[i];
            uint32_t rec_len = sizeof(struct virtfs_dir_entry) + entry->nlen;
            if(entry->nlen == 0 || rec_len > (uint32_t)(ret - i))
                break;

            i += rec_len;
            offset = entry->offset;
            if(virtfs_dirent_is_dots(entry))
                continue;
            if(entry->type != VIRTFS_TYPE_DIR && entry->type != VIRTFS_TYPE_FILE)
                continue;

            debug_entry(entry);
            int new_fid = alloc_node(fid);
            if(new_fid < 0)
                continue;
            int nlen = MIN(entry->nlen, sizeof(name) - 1);
            memcpy(name, entry->name, nlen);
            name[nlen] = '\0';
            if(virtfs_walk(fs, 1, fid, (uint32_t)new_fid, name) != 0)
                continue;

            fsinfo_t finfo;
            if(virtfs_fill_info(&finfo, entry, (uint32_t)new_fid, fs) != 0)
                continue;

            fsinfo_t* new_infos = realloc(infos, sizeof(fsinfo_t) * (count + 1));
            if(new_infos == NULL)
                continue;
            infos = new_infos;
            memcpy(&infos[count], &finfo, sizeof(fsinfo_t));
            count++;
        }

        if(ret < (int)sizeof(dir))
            break;
    }

    *num = count;
    return infos;
}

static int _mount(vdevice_t *dev, fsinfo_t *info, void *p)
{
    (void)dev;
    virtfs_t fs = (virtfs_t)p;
    int root = alloc_node(0);
    if(root < 0)
        return -1;
    FS_DBG("fid:%d\n", root);
    if(virtfs_attach(fs, 2, root, 0xffffffff, "root", "/") != 0)
        return -1;

    info->data = (uint32_t)root;
    info->state |= FS_STATE_KIDS_LOADED;

    uint32_t num = 0;
    fsinfo_t* kids = virtfsd_list_directory(fs, (uint32_t)root, &num);
    for(uint32_t off = 0; off < num; off += NEW_NODES_BATCH) {
        uint32_t n = num - off;
        if(n > NEW_NODES_BATCH)
            n = NEW_NODES_BATCH;
        if(vfs_new_nodes(&kids[off], n, info->node) != 0) {
            for(uint32_t j = 0; j < n; j++)
                vfs_new_node(&kids[off+j], info->node, false, false);
        }
    }
    free(kids);
    return 0;
}

static int _create(vdevice_t *dev, int pid, fsinfo_t *info_to, fsinfo_t *info, void *p)
{
    (void)dev;
    FS_DBG("virtfsd create %s\n", info->name);
    return 0;
}

static int _open(vdevice_t *dev, int fd, int from_pid, fsinfo_t *info, int oflag, void *p)
{
    (void)dev;
    (void)fd;
    (void)from_pid;
    FS_DBG("virtfsd open %s %d flag:%08x\n", info->name, info->data, oflag);
    virtfs_t fs = (virtfs_t)p;
    return virtfs_ensure_open(fs, (uint32_t)info->data, oflag);
}

static int _stat(vdevice_t *dev, int from_pid, fsinfo_t *info, node_stat_t *stat, void *p)
{
    (void)dev;
    FS_DBG("virtfsd stat %s %d\n", info->name, info->data);
    virtfs_t fs = (virtfs_t)p;
    virtfs_stat_t st;
    if (FS_IS_TYPE(info->type, FS_TYPE_DIR))
        return 0;
    if (virtfs_stat(fs, 1, info->data, &st) == 0)
    {
        stat->size = st.length;
        stat->mode = st.mode | 0444;
        stat->atime = st.atime;
        stat->mtime = st.mtime;
        stat->gid = 100;
        stat->uid = 100;
        stat->mtime = st.mtime;
    }
    return 0;
}

static int _set(vdevice_t *dev, int from_pid, fsinfo_t *info, void *p)
{
    (void)dev;
    (void)from_pid;
    (void)info;
    (void)p;
    FS_DBG("virtfsd set %s\n", info->name);
    return 0;
}

static fsinfo_t* _kids(vdevice_t *dev, fsinfo_t *info_dir, uint32_t *num, void *p)
{
    (void)dev;
    if(!FS_IS_TYPE(info_dir->type, FS_TYPE_DIR)) {
        *num = 0;
        return NULL;
    }
    return virtfsd_list_directory((virtfs_t)p, (uint32_t)info_dir->data, num);
}

static int _read(vdevice_t *dev, int fd, int from_pid, fsinfo_t *info,
                 void *buf, int size, int offset, void *p)
{
    (void)dev;
    (void)fd;
    (void)from_pid;
    FS_DBG("virtfsd read %s %d offset:%d size:%d\n", info->name, info->data, offset, size);
    virtfs_t fs = (virtfs_t)p;
    if (virtfs_ensure_open(fs, (uint32_t)info->data, O_RDONLY) != 0)
    {
        return -1;
    }
    return virtfs_read(fs, 1, info->data, buf, offset, size);
}

static int _write(vdevice_t *dev, int fd, int from_pid, fsinfo_t *info,
                  const void *buf, int size, int offset, void *p)
{
    (void)dev;
    (void)fd;
    (void)from_pid;
    FS_DBG("virtfsd write %s %d\n", info->name, info->data);
    return size;
}

static int _unlink(vdevice_t *dev, fsinfo_t *info, const char *fname, void *p)
{
    (void)dev;
    FS_DBG("virtfsd unlink %s %d\n", fname, info->data);
    return 0;
}

static int _close(vdevice_t *dev, int fd, int from_pid, ewokos_addr_t node, fsinfo_t* info, void* p)
{
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)node;
    FS_DBG("virtfsd close %s %d\n", info->name, info->data);
    virtfs_t fs = (virtfs_t)p;
    return virtfs_sync(fs, 1, info->data);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    _mmio_base = mmio_map();
    virtfs_t fs = virtfs_init();
    if (fs == NULL)
    {
        FS_DBG("virtfs_init failed!\n");
        return -1;
    }
    virtfs_set_version(fs, "9P2000.L", VIRTFS_DEFAULT_MSIZE);

    vdevice_t dev;
    memset(&dev, 0, sizeof(vdevice_t));
    strcpy(dev.name, "sharedfs(virtfs)");

    dev.mount = _mount;
    dev.read = _read;
    dev.stat = _stat;
    dev.write = _write;
    dev.create = _create;
    dev.open = _open;
    dev.close = _close;
    dev.set = _set;
    dev.kids = _kids;
    dev.unlink = _unlink;

    dev.extra_data = fs;
    device_run(&dev, "/mnt/share", FS_TYPE_DIR, 0777);
    return 0;
}
