#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <ewoksys/wait.h>
#include <string.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <ewoksys/klog.h>
#include <ewoksys/usbmsc.h>
#include <sysinfo.h>
#include <fcntl.h>
#include <sd/sd.h>
#include <sd/partition.h>
#include <fat32/fat32fs.h>
#include <stdio.h>
#include <bsp/bsp_sd.h>

static uint32_t _partition_start = 0;
static fat32_node_t _root_node;
static fat32_t _fat;
static vdevice_t* _dev = NULL;

/* USB block mode: sectors are transported to usbhostd over
   FS_CMD_DEV_CNTL instead of going through the SD driver */
static bool _usb_mode = false;
static int _usb_dev_pid = -1;
static uint32_t _usb_io_fail = 0;
#define USB_IO_FAIL_STOP 8u

static int32_t fat_sd_read_sector(int32_t sector, void* buf) {
    return (sd_read_sector(sector + (int32_t)_partition_start, buf) == SECTOR_SIZE) ? 0 : -1;
}

static int32_t fat_sd_read_sectors(int32_t sector, void* buf, uint32_t count) {
    return (sd_read_sectors(sector + (int32_t)_partition_start, buf, count) ==
            (int32_t)(count * SECTOR_SIZE)) ? 0 : -1;
}

static int32_t fat_sd_write_sector(int32_t sector, const void* buf) {
    return (sd_write_sector(sector + (int32_t)_partition_start, buf) == SECTOR_SIZE) ? 0 : -1;
}

/* USB-mode sector transport: one dev_cntl round-trip to usbhostd per call.
   Repeated failures mean the device was unplugged without umount: stop the
   daemon so device_run tears the mount down. */
static int32_t fat_usb_io(bool wr, int32_t sector, void* buf, uint32_t count) {
    proto_t in, out;
    int ret = -1;

    PF->init(&in)->
        addi(&in, (ewokos_addr_t)(sector + (int32_t)_partition_start))->
        addi(&in, count);
    if(wr)
        PF->add(&in, buf, count * SECTOR_SIZE);
    PF->init(&out);

    int res = dev_cntl_by_pid(_usb_dev_pid, wr ? USBMSC_CMD_WRITE : USBMSC_CMD_READ, &in, &out);
    PF->clear(&in);
    if(res == 0) {
        if(wr) {
            ret = 0;
        }
        else {
            int32_t sz = 0;
            void* data = proto_read(&out, &sz);
            if(data != NULL && sz == (int32_t)(count * SECTOR_SIZE)) {
                memcpy(buf, data, sz);
                ret = 0;
            }
        }
    }
    PF->clear(&out);

    if(ret != 0) {
        if(++_usb_io_fail >= USB_IO_FAIL_STOP && _dev != NULL) {
            klog("fat32fsd: usb device not responding, stopping\n");
            device_stop(_dev);
        }
    }
    else
        _usb_io_fail = 0;
    return ret;
}

static int32_t fat_usb_read_sector(int32_t sector, void* buf) {
    return fat_usb_io(false, sector, buf, 1);
}

static int32_t fat_usb_read_sectors(int32_t sector, void* buf, uint32_t count) {
    uint8_t* p = (uint8_t*)buf;
    while(count > 0) {
        uint32_t n = (count > USBMSC_MAX_SECTORS) ? USBMSC_MAX_SECTORS : count;
        if(fat_usb_io(false, sector, p, n) != 0)
            return -1;
        sector += (int32_t)n;
        p += n * SECTOR_SIZE;
        count -= n;
    }
    return 0;
}

static int32_t fat_usb_write_sector(int32_t sector, const void* buf) {
    return fat_usb_io(true, sector, (void*)buf, 1);
}

static void fat_flush_device(void) {
    if(_usb_mode) {
        proto_t in, out;
        PF->init(&in);
        PF->init(&out);
        dev_cntl_by_pid(_usb_dev_pid, USBMSC_CMD_FLUSH, &in, &out);
        PF->clear(&in);
        PF->clear(&out);
    }
    else {
        sd_flush();
    }
}

static fat32_node_t* node_of(fsinfo_t* info) {
    fat32_node_t* node = (fat32_node_t*)(ewokos_addr_t)info->data;
    return (node == NULL) ? &_root_node : node;
}

static void set_fsinfo_stat(node_stat_t* stat, fat32_node_t* node) {
    uint32_t ctime = fat32_dt2unix(node->crt_date, node->crt_time);
    stat->atime = ctime;
    stat->ctime = ctime;
    stat->mtime = fat32_dt2unix(node->wrt_date, node->wrt_time);
    stat->gid = 0;
    stat->uid = 0;
    stat->links_count = 1;
    //FAT has no unix permissions, expose rw unless the readonly attr is set
    uint16_t perm = (node->attr & FAT32_ATTR_READ_ONLY) ? 0555 : 0777;
    stat->mode = ((node->attr & FAT32_ATTR_DIRECTORY) ? 0x4000 : 0x8000) | perm;
    stat->size = node->size;
}

static void fill_kid_info(fsinfo_t* f, fat32_node_t* node) {
    uint32_t len = strlen(node->name);
    if(len >= FS_NODE_NAME_MAX)
        len = FS_NODE_NAME_MAX - 1; //VFS names are shorter than FAT LFNs, truncate
    memset(f, 0, sizeof(fsinfo_t));
    memcpy(f->name, node->name, len);
    f->type = (node->attr & FAT32_ATTR_DIRECTORY) ? FS_TYPE_DIR : FS_TYPE_FILE;
    f->data = (ewokos_addr_t)node;
    set_fsinfo_stat(&f->stat, node);
}

#define NEW_NODES_BATCH 64

static int32_t add_nodes(fat32_t* fat, fat32_node_t* dir_node, fsinfo_t* dinfo) {
    fat32_dir_t it;
    fat32_node_t kid;

    fsinfo_t* kids = NULL;
    uint32_t kid_num = 0;
    uint32_t kid_cap = 0;

    if(fat32_diropen(fat, dir_node, &it) != 0)
        return -1;

    while(fat32_dirnext(fat, &it, &kid) == 1) {
        fat32_node_t* node = (fat32_node_t*)malloc(sizeof(fat32_node_t));
        if(node == NULL)
            break;
        memcpy(node, &kid, sizeof(fat32_node_t));

        if(kid_num >= kid_cap) { //grow geometrically, one realloc per entry is O(n^2)
            kid_cap = (kid_cap == 0) ? 16 : (kid_cap * 2);
            kids = realloc(kids, sizeof(fsinfo_t) * kid_cap);
        }
        fill_kid_info(&kids[kid_num], node);
        kid_num++;
    }
    fat32_dirclose(&it);

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

static int fat32fs_mount(vdevice_t* dev, fsinfo_t* info, void* p) {
    (void)dev;
    fat32_t* fat = (fat32_t*)p;
    info->state |= FS_STATE_KIDS_LOADED;
    fat32_root_node(fat, &_root_node);
    add_nodes(fat, &_root_node, info);
    return 0;
}

/* flush everything to the media before the mount is detached */
static int fat32fs_umount(vdevice_t* dev, ewokos_addr_t node, void* p) {
    (void)dev;
    (void)node;
    fat32_t* fat = (fat32_t*)p;
    fat32_flush(fat);
    fat_flush_device();
    return 0;
}

/* umount command from userspace / quit hint from usbhostd on unplug */
static int fat32fs_dev_cntl(vdevice_t* dev, int from_pid, int cmd,
        proto_t* in, proto_t* out, void* p) {
    (void)from_pid;
    (void)in;
    (void)out;
    fat32_t* fat = (fat32_t*)p;

    switch(cmd) {
    case USBFS_CMD_UMOUNT:
        fat32_flush(fat);
        fat_flush_device();
        /* this daemon owns the mount, so the detach is permitted; vfsd
           defers it while files are still open, which reads as "busy" */
        if(vfs_umount(dev->mnt_info.node) != 0)
            return -1;
        device_stop(dev);
        return 0;
    case USBFS_CMD_QUIT:
        fat32_flush(fat);
        device_stop(dev);
        return 0;
    default:
        return -1;
    }
}

static int fat32fs_create(vdevice_t* dev, int pid, fsinfo_t* info_to, fsinfo_t* info, void* p) {
    (void)dev;
    (void)pid;
    fat32_t* fat = (fat32_t*)p;
    fat32_node_t* dir_node = node_of(info_to);

    fat32_node_t* node = (fat32_node_t*)malloc(sizeof(fat32_node_t));
    if(node == NULL)
        return -1;

    int res;
    if(FS_IS_TYPE(info->type, FS_TYPE_DIR))
        res = fat32_create_dir(fat, dir_node, info->name, node);
    else
        res = fat32_create_file(fat, dir_node, info->name, node);

    if(res != 0) {
        free(node);
        return -1;
    }

    info->data = (ewokos_addr_t)node;
    set_fsinfo_stat(&info->stat, node);
    if(FS_IS_TYPE(info->type, FS_TYPE_DIR))
        info->state |= FS_STATE_KIDS_LOADED;
    return 0;
}

static int fat32fs_open(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info, int oflag, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;

    if((oflag & O_TRUNC) == 0)
        return 0;

    fat32_t* fat = (fat32_t*)p;
    fat32_node_t* node = node_of(info);
    if(node == &_root_node)
        return -1;

    if(fat32_truncate(fat, node) != 0)
        return -1;
    set_fsinfo_stat(&info->stat, node);
    return 0;
}

static int fat32fs_set(vdevice_t* dev, int from_pid, fsinfo_t* info, void* p) {
    (void)dev;
    (void)from_pid;
    fat32_t* fat = (fat32_t*)p;
    fat32_node_t* node = node_of(info);
    if(node == &_root_node)
        return -1;

    //FAT only records a readonly bit, keep the dir/archive attrs untouched
    if((info->stat.mode & 0200) == 0)
        node->attr |= FAT32_ATTR_READ_ONLY;
    else
        node->attr &= ~FAT32_ATTR_READ_ONLY;
    if(info->stat.mtime != 0)
        fat32_unix2dt(info->stat.mtime, &node->wrt_date, &node->wrt_time);
    if((node->attr & FAT32_ATTR_DIRECTORY) == 0)
        node->size = info->stat.size;
    return fat32_update_node(fat, node);
}

static int fat32fs_get(vdevice_t* dev, int from_pid, const char* fname, fsinfo_t* info, void* p) {
    (void)dev;
    (void)from_pid;
    fat32_t* fat = (fat32_t*)p;

    fat32_node_t* node = (fat32_node_t*)malloc(sizeof(fat32_node_t));
    if(node == NULL)
        return -1;
    if(fat32_node_by_fname(fat, fname, node) != 0) {
        free(node);
        return -1;
    }
    fill_kid_info(info, node);
    return 0;
}

static fsinfo_t* fat32fs_kids(vdevice_t* dev, fsinfo_t* info_dir, uint32_t* num, void* p) {
    (void)dev;
    fsinfo_t* ret = NULL;
    *num = 0;
    if(!FS_IS_TYPE(info_dir->type, FS_TYPE_DIR))
        return NULL;

    fat32_t* fat = (fat32_t*)p;
    fat32_node_t* dir_node = node_of(info_dir);
    fat32_dir_t it;
    fat32_node_t kid;

    if(fat32_diropen(fat, dir_node, &it) != 0)
        return NULL;

    while(fat32_dirnext(fat, &it, &kid) == 1) {
        fat32_node_t* node = (fat32_node_t*)malloc(sizeof(fat32_node_t));
        if(node == NULL)
            break;
        memcpy(node, &kid, sizeof(fat32_node_t));

        ret = realloc(ret, sizeof(fsinfo_t) * (*num + 1));
        fill_kid_info(&ret[*num], node);
        (*num)++;
    }
    fat32_dirclose(&it);
    return ret;
}

static int fat32fs_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
        void* buf, int size, int offset, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;

    fat32_t* fat = (fat32_t*)p;
    fat32_node_t* node = node_of(info);

    int rsize = info->stat.size - offset;
    if(rsize < size)
        size = rsize;
    if(size < 0)
        size = -1;

    if(size > 0)
        size = fat32_read(fat, node, buf, size, offset);
    return size;
}

static int fat32fs_write(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
        const void* buf, int size, int offset, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;

    fat32_t* fat = (fat32_t*)p;
    fat32_node_t* node = node_of(info);
    if(node == &_root_node)
        return -1;

    size = fat32_write(fat, node, buf, size, offset);
    if(size >= 0) {
        set_fsinfo_stat(&info->stat, node);
        fat32_update_node(fat, node);
    }
    return size;
}

static int fat32fs_unlink(vdevice_t* dev, fsinfo_t* info, const char* fname, void* p) {
    (void)dev;
    fat32_t* fat = (fat32_t*)p;
    int ret = FS_IS_TYPE(info->type, FS_TYPE_DIR) ? fat32_rmdir(fat, fname) : fat32_unlink(fat, fname);
    if(ret != 0)
        return -1;

    fat32_node_t* node = (fat32_node_t*)(ewokos_addr_t)info->data;
    if(node != NULL && node != &_root_node)
        free(node);
    return vfs_del_node(info->node);
}

/* locate the FAT32 partition (0x0B/0x0C) in the MBR;
 * fall back to a partition-less (superfloppy) layout. */
static int32_t find_fat32_partition(int32_t forced_index) {
    if(read_partition() == 0) {
        for(uint32_t i = 0; i < 4; i++) {
            partition_t part;
            if(partition_get(i, &part) != 0)
                continue;
            if(forced_index >= 0 && (uint32_t)forced_index != i)
                continue;
            if(part.sys_type == 0x0B || part.sys_type == 0x0C) {
                _partition_start = part.start_sector;
                return 0;
            }
        }
        if(forced_index >= 0)
            return -1;
    }
    _partition_start = 0;
    return 0;
}

/* same probe over the USB sector transport (libsd is not initialized) */
static int32_t find_fat32_partition_usb(int32_t forced_index) {
    uint8_t mbr[SECTOR_SIZE];

    if(fat_usb_io(false, 0, mbr, 1) != 0) {
        klog("fat32fsd: usb read mbr failed!\n");
        return -1;
    }
    if(mbr[510] == 0x55 && mbr[511] == 0xAA) {
        for(uint32_t i = 0; i < 4; i++) {
            const uint8_t* e = mbr + 446 + i * 16;
            if(forced_index >= 0 && (uint32_t)forced_index != i)
                continue;
            if(e[4] == 0x0B || e[4] == 0x0C) {
                _partition_start = (uint32_t)e[8] | ((uint32_t)e[9] << 8) |
                        ((uint32_t)e[10] << 16) | ((uint32_t)e[11] << 24);
                return 0;
            }
        }
        if(forced_index >= 0)
            return -1;
    }
    _partition_start = 0;
    return 0;
}

/* wait until usbhostd reports the mass-storage device ready */
static int wait_usb_device_ready(void) {
    for(int i = 0; i < 30; i++) {
        proto_t in, out;
        PF->init(&in);
        PF->init(&out);
        int res = dev_cntl_by_pid(_usb_dev_pid, USBMSC_CMD_INFO, &in, &out);
        PF->clear(&in);
        int present = (res == 0 && proto_read_int(&out) == 1) ? 1 : 0;
        PF->clear(&out);
        if(present)
            return 0;
        proc_usleep(100000);
    }
    return -1;
}

int main(int argc, char** argv) {
    const char* mnt_point = argc > 1 ? argv[1] : "/mnt/fat32";
    int32_t part_index = argc > 2 ? atoi(argv[2]) : -1;

    if(argc > 2 && strcmp(argv[1], "-u") == 0) {
        /* USB block mode: fat32fsd -u <usbdev> [mnt_point] */
        const char* dev_path = argv[2];
        mnt_point = argc > 3 ? argv[3] : "/mnt/udisk0";
        part_index = argc > 4 ? atoi(argv[4]) : -1;
        _usb_mode = true;

        fsinfo_t dev_info;
        if(vfs_get_by_name(dev_path, &dev_info) != 0) {
            klog("fat32fsd: usb dev %s not found!\n", dev_path);
            return -1;
        }
        _usb_dev_pid = dev_info.mount_pid;

        if(wait_usb_device_ready() != 0) {
            klog("fat32fsd: usb device not ready!\n");
            return -1;
        }

        if(find_fat32_partition_usb(part_index) != 0) {
            klog("fat32fsd: no fat32 partition found!\n");
            return -1;
        }

        if(fat32_init_ex(&_fat, fat_usb_read_sector, fat_usb_read_sectors, fat_usb_write_sector) != 0) {
            klog("fat32fsd: not a valid fat32 volume!\n");
            return -1;
        }
    }
    else {
        if(bsp_sd_init() != 0) {
            klog("fat32fsd: sd init failed!\n");
            return -1;
        }

        if(find_fat32_partition(part_index) != 0) {
            klog("fat32fsd: no fat32 partition found!\n");
            sd_quit();
            return -1;
        }

        if(fat32_init_ex(&_fat, fat_sd_read_sector, fat_sd_read_sectors, fat_sd_write_sector) != 0) {
            klog("fat32fsd: not a valid fat32 volume!\n");
            sd_quit();
            return -1;
        }
    }

    vdevice_t dev;
    memset(&dev, 0, sizeof(vdevice_t));
    strcpy(dev.desc, "fat32fs");
    dev.mount = fat32fs_mount;
    dev.umount = fat32fs_umount;
    dev.dev_cntl = fat32fs_dev_cntl;
    dev.read = fat32fs_read;
    dev.write = fat32fs_write;
    dev.create = fat32fs_create;
    dev.open = fat32fs_open;
    dev.set = fat32fs_set;
    dev.get = fat32fs_get;
    dev.kids = fat32fs_kids;
    dev.unlink = fat32fs_unlink;

    dev.extra_data = &_fat;
    _dev = &dev;
    device_run(&dev, mnt_point, FS_TYPE_DIR, 0777, false);
    fat32_quit(&_fat);
    if(!_usb_mode)
        sd_quit();
    return 0;
}
