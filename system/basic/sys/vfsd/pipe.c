/*
 * pipe.c - fd-close ref accounting (proc_file_close).
 *
 * Anonymous pipes live in the standalone pipe driver
 * (system/basic/drivers/piped): the shm ring lifecycle, all read/write data
 * transfer, block/wake delivery and poll waiters are owned there. From
 * vfsd's point of view a pipe end is just an anonymous char device node, so
 * closing it only takes the generic refcount + node teardown path. The old
 * in-vfsd pipe IPC handlers (VFS_PIPE_OPEN/READ/WRITE) and their buffer_t /
 * shm_ring state have been removed together with the fifo backend.
 */
#include "vfsd.h"

/* caller must hold _vfs_lock (write) */
void proc_file_close(int pid, int fd, file_t* file) {
    (void)pid;
    (void)fd;
    if(file == NULL || file->node == NULL)
        return;
    vfs_node_t* node = file->node;

    if(node->refs > 0)
        node->refs--;
    if((file->flags & (O_WRONLY|O_RDWR)) != 0 && node->refs_w > 0)
        node->refs_w--;
    bool del_node = false;
    if(FS_IS_ANONYMOUS(node->fsinfo.type)) {
        if(node->refs <= 0) {
            del_node = true;
            file->node = 0;
            do_node_wakeup(node, VFS_EVT_CLOSE);
        }
    }

    if(del_node)
        vfsd_del_node(node);
    else
        vfs_try_finish_umount(node);
    file->node = NULL;
}
