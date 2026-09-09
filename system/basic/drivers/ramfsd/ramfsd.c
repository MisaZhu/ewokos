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

typedef struct {
    char* buf;
    size_t size;
    size_t capacity;
} ramfs_file_t;

static ramfs_file_t* ramfs_get_file(fsinfo_t* info) {
    return (ramfs_file_t*)(ewokos_addr_t)info->data;
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

static ramfs_file_t* ramfs_file_create(void) {
    ramfs_file_t* file = (ramfs_file_t*)malloc(sizeof(ramfs_file_t));

    if(file == NULL)
        return NULL;
    memset(file, 0, sizeof(*file));
    return file;
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
     * ext3_truncate in its open hook. */
    if((oflag & O_TRUNC) != 0) {
        ramfs_file_t* file = ramfs_get_file(info);
        if(file != NULL)
            file->size = 0;
        info->stat.size = 0;
    }
    return 0;
}

static int ramfs_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
        void* buf, int size, int offset, void* p) {
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
    return size;	
}

static int ramfs_write(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
        const void* buf, int size, int offset, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)p;
    ramfs_file_t* file = ramfs_get_file(info);
    size_t end;

    if(offset < 0)
        offset = 0;
    if(size <= 0)
        return size;
    if(file == NULL) {
        file = ramfs_file_create();
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
    info->stat.size = file->size;
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

int main(int argc, char** argv) {
    const char* mnt_point = argc > 1 ? argv[1]: "/tmp";

    vdevice_t dev;
    memset(&dev, 0, sizeof(vdevice_t));
    strcpy(dev.desc, "ramfs");
    dev.open = ramfs_open;
    dev.read = ramfs_read;
    dev.write = ramfs_write;
    dev.unlink = ramfs_unlink;
    
    device_run(&dev, mnt_point, FS_TYPE_DIR, 0777, false);
    return 0;
}
