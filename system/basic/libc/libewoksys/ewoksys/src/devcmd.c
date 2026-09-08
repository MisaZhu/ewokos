#include <ewoksys/devcmd.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vfs.h>
#include <sys/shm.h>
#include <sys/errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

int dev_set(int dev_pid, fsinfo_t* info) {
    proto_t in, out;
    PF->init(&out);
    PF->init(&in)->add(&in, info, sizeof(fsinfo_t));

    int res = ipc_call(dev_pid, FS_CMD_SET, &in, &out);
    PF->clear(&in);
    if(res != 0) {
        PF->clear(&out);
        return res;
    }

    res = proto_read_int(&out);
    if(res != 0)
        errno = proto_read_int(&out);
    PF->clear(&out);
    return res;
}

int dev_stat(int dev_pid,  fsinfo_t* info, node_stat_t* stat) {
    proto_t in, out;
    PF->init(&out);
    PF->init(&out);
    PF->init(&in)->add(&in, info, sizeof(fsinfo_t));
    int res = ipc_call(dev_pid, FS_CMD_STAT, &in, &out);
    PF->clear(&in);
    if(res != 0) {
        PF->clear(&out);
        return res;
    }
    res = proto_read_int(&out);
    if(res == 0)
        proto_read_to(&out, stat, sizeof(node_stat_t));

    PF->clear(&in);
    PF->clear(&out);
    return res;
}

int dev_get_by_name(int dev_pid, const char* fname, fsinfo_t* info) {
    int res;
    proto_t in, out;
    PF->init(&in)->adds(&in, fname);
    PF->init(&out);
    res = ipc_call(dev_pid, FS_CMD_GET, &in, &out);
    PF->clear(&in);
    if(res == 0) {
        res = proto_read_int(&out); //res = node
        if(res == 0) {
            if(info != NULL){
                proto_read_to(&out, info, sizeof(fsinfo_t));
                //fix me: update stat form device
                //dev_stat(info->mount_pid, info, &info->stat);
            }
        }
        else
            res = -1;
    }
    PF->clear(&out);
    return res;	
}

fsinfo_t* dev_kids(int dev_pid, fsinfo_t* info, uint32_t *num) {
    fsinfo_t* ret = NULL;
    if(info == NULL)
        return NULL;

    proto_t in, out;
    PF->init(&out);
    PF->init(&in)->
        addi(&in, info->node)->
        add(&in, info, sizeof(fsinfo_t));
    int res = ipc_call(info->mount_pid, FS_CMD_KIDS, &in, &out);	
    if(res == 0) {
        uint32_t n = proto_read_int(&out);
        *num = n;
        if(n > 0) {
            ret = (fsinfo_t*)malloc(n * sizeof(fsinfo_t));
            proto_read_to(&out, ret, n * sizeof(fsinfo_t));
        }
    }
    PF->clear(&out);
    PF->clear(&in);
    return ret;
}

int dev_unlink(int dev_pid, ewokos_addr_t node, const char* fname) {
    proto_t in, out;
    PF->init(&out);
    PF->init(&in)->addi(&in, node)->adds(&in, fname);

    int res = ipc_call(dev_pid, FS_CMD_UNLINK, &in, &out);
    PF->clear(&in);
    if(res != 0) {
        PF->clear(&out);
        return res;
    }

    res = proto_read_int(&out);
    if(res != 0)
        errno = proto_read_int(&out);
    PF->clear(&out);
    return res;
}

int dev_open(int dev_pid, int fd, fsinfo_t* info, int oflag) {
    proto_t in, out;
    PF->init(&out);
    /* Ship the whole fsinfo the vfsd just handed us: the driver then needs no
     * extra VFS_GET_BY_NODE round trip, which also closes the window where an
     * announimous node created by VFS_OPEN is not visible yet/anymore. */
    PF->init(&in)->
        addi(&in, fd)->
        addi(&in, info->node)->
        add(&in, info, sizeof(fsinfo_t))->
        addi(&in, oflag);

    int res = ipc_call(dev_pid, FS_CMD_OPEN, &in, &out);
    PF->clear(&in);
    if(res != 0) {
        PF->clear(&out);
        return res;
    }
    res =	proto_read_int(&out);
    if(res != 0)
        errno = proto_read_int(&out);
    else
        proto_read_to(&out, info, sizeof(fsinfo_t));

    PF->clear(&in);
    PF->clear(&out);
    return res;
}

#define SHM_ON  256
#define SHM_MAX (1024*256)

/*
 * Per-fd persistent transfer buffer cache.
 *
 * Previously dev_read/dev_write created (shmget IPC_EXCL), mapped (shmat) and
 * destroyed (shmdt) a shared-memory segment on *every* large IO. Each map/unmap
 * triggers a global TLB flush plus page alloc/free in the kernel, so a single
 * 64KB transfer paid a full segment create/map/unmap/destroy lifecycle and
 * 4-6 TLB flushes. Cache one segment per fd and reuse it across calls; the
 * segment is freed on close (dev_io_on_close) and its stale pointer dropped
 * after fork (dev_io_on_fork), mirroring vfs.c's _pipe_shm[] handling.
 */
typedef struct {
    int32_t  shm_id;
    void*    addr;
    uint32_t size;
} io_shm_t;

static io_shm_t _io_shm[MAX_OPEN_FILE_PER_PROC];

static void* get_io_shm(int fd, uint32_t size) {
    if(fd < 0 || fd >= MAX_OPEN_FILE_PER_PROC)
        return NULL;
    (void)size;

    io_shm_t* c = &_io_shm[fd];
    if(c->addr != NULL)
        return c->addr;

    /*
     * Always allocate the segment at the maximum transfer size. A keyed
     * segment can outlive this fd's close() while a server still holds a
     * mapping, and shmget() returns the existing segment WITHOUT resizing;
     * fixing the size at SHM_MAX makes every (re)use large enough and removes
     * any grow/overflow path.
     *
     * Keyed (non-private) 0666 segment: the fs/device server is an unrelated
     * process and cannot map an IPC_PRIVATE (family-only) segment. Key is
     * unique per (fd, pid) and stable across calls so the server-side mapping
     * cache stays warm. Drop IPC_EXCL so a re-get returns the existing id.
     */
    key_t key = ((key_t)(fd + 1) << 16) | (getpid() & 0xffff);
    int32_t shm_id = shmget(key, SHM_MAX, 0666|IPC_CREAT);
    if(shm_id == -1)
        return NULL;
    void* shm = shmat(shm_id, 0, 0);
    if(shm == (void*)-1) {
        /* destroy the segment when nobody has it attached (a leftover
           from an earlier failed attempt gets cleaned up the same way) */
        shmctl(shm_id, IPC_RMID, NULL);
        return NULL;
    }

    c->shm_id = shm_id;
    c->addr = shm;
    c->size = SHM_MAX;
    return shm;
}

void dev_io_on_close(int fd) {
    if(fd < 0 || fd >= MAX_OPEN_FILE_PER_PROC)
        return;
    io_shm_t* c = &_io_shm[fd];
    if(c->addr != NULL)
        shmdt(c->addr);
    c->addr = NULL;
    c->shm_id = -1;
    c->size = 0;
}

void dev_io_on_fork(void) {
    /* child never mapped these pages; just drop the stale pointers */
    for(uint32_t i=0; i<MAX_OPEN_FILE_PER_PROC; i++) {
        _io_shm[i].addr = NULL;
        _io_shm[i].shm_id = -1;
        _io_shm[i].size = 0;
    }
}

int dev_read(int dev_pid, int fd, fsinfo_t* info, int32_t offset, void* buf, uint32_t size) {
    int32_t shm_id = -1;
    void* shm = NULL;
    if(size > SHM_ON) {
        if(size > SHM_MAX)
            size = SHM_MAX;
        shm = get_io_shm(fd, size);
        if(shm != NULL)
            shm_id = _io_shm[fd].shm_id;
    }

    proto_t in, out;
    PF->init(&out);
    PF->init(&in)->
        addi(&in, fd)->
        addi(&in, info->node)->
        addi(&in, size)->
        addi(&in, offset)->
        addi(&in, shm_id)->
        add(&in, info, sizeof(fsinfo_t));

    int res = -1;
    if(ipc_call(dev_pid, FS_CMD_READ, &in, &out) == 0) {
        int rd = proto_read_int(&out);
        res = rd;
        if(rd > 0) {
            if(shm_id != -1 && shm != NULL)
                memcpy(buf, shm, rd);
            else
                proto_read_to(&out, buf, rd);
        }
        else if(rd < 0 && out.size > out.offset) {
            errno = proto_read_int(&out);
        }
    }
    PF->clear(&in);
    PF->clear(&out);
    return res;
}

int dev_write(int dev_pid, int fd, fsinfo_t* info, int32_t offset, const void* buf, uint32_t size) {
    int32_t shm_id = -1;
    void* shm = NULL;
    if(size >= SHM_ON) {
        if(size > SHM_MAX)
            size = SHM_MAX;
        shm = get_io_shm(fd, size);
        if(shm != NULL) {
            shm_id = _io_shm[fd].shm_id;
            memcpy(shm, buf, size);
        }
    }

    proto_t in, out;
    PF->init(&out);
    PF->init(&in)->
        addi(&in, fd)->
        addi(&in, info->node)->
        addi(&in, offset)->
        addi(&in, shm_id)->
        add(&in, info, sizeof(fsinfo_t));
    if(shm_id == -1)
        PF->add(&in, buf, size);
    else
        PF->addi(&in, size);

    int res = -1;
    if(ipc_call(dev_pid, FS_CMD_WRITE, &in, &out) == 0) {
        int r = proto_read_int(&out);
        proto_read_to(&out, info, sizeof(fsinfo_t));
        res = r;
        if(r < 0 && out.size > out.offset) {
            errno = proto_read_int(&out);
        }
    }
    PF->clear(&in);
    PF->clear(&out);
    return res;
}

int dev_create(int dev_pid, fsinfo_t* info_to, fsinfo_t* info) {
    proto_t in, out;
    PF->init(&out);
    PF->init(&in)->addi(&in, info_to->node)->addi(&in, info->node);

    int res = -1;
    if(ipc_call(dev_pid, FS_CMD_CREATE, &in, &out) == 0) {
        res = proto_read_int(&out);
        if(res == 0)
            proto_read_to(&out, info, sizeof(fsinfo_t));
    }
    PF->clear(&in);
    PF->clear(&out);
    return res;
}

int dev_poll(int dev_pid, int fd, fsinfo_t* info, uint32_t* events) {
    proto_t in, out;
    PF->init(&out);
    PF->init(&in)->
        addi(&in, fd)->
        addi(&in, info->node)->
        add(&in, info, sizeof(fsinfo_t));

    int res = -1;
    if(ipc_call(dev_pid, FS_CMD_POLL, &in, &out) == 0) {
        res = proto_read_int(&out);
        if(res == 0 && events != NULL) {
            *events = (uint32_t)proto_read_int(&out);
        }
    }
    PF->clear(&in);
    PF->clear(&out);
    return res;
}

int dev_fcntl(int dev_pid, int fd, fsinfo_t* info, int cmd, proto_t* arg_in, proto_t* arg_out) {
    proto_t in;
    PF->init(&in)->
        addi(&in, fd)->
        addi(&in, info->node)->
        addi(&in, cmd)->
        add(&in, info, sizeof(fsinfo_t));
    if(arg_in == NULL)
        PF->add(&in, NULL, 0);
    else
        PF->add(&in, arg_in->data, arg_in->size);

    int res = -1;
    proto_t out;
    PF->init(&out);
    if(arg_out == NULL)
        res = ipc_call(dev_pid, FS_CMD_CNTL, &in, NULL);
    else
        res = ipc_call(dev_pid, FS_CMD_CNTL, &in, &out);

    if(res == 0) {
        res = proto_read_int(&out);
        if(arg_out != NULL) {
            proto_read_to(&out, info, sizeof(fsinfo_t));
            int32_t sz;
            void *p = proto_read(&out, &sz);
            PF->copy(arg_out, p, sz);
        }
    }
    PF->clear(&in);
    PF->clear(&out);
    return res;
}

int dev_flush(int dev_pid, int fd, ewokos_addr_t node, int8_t wait) {
    proto_t in;
    PF->init(&in)->addi(&in, fd)->addi(&in, node);

    int res = -1;
    if(wait)
        res = ipc_call_wait(dev_pid, FS_CMD_FLUSH, &in);
    else
        res = ipc_call(dev_pid, FS_CMD_FLUSH, &in, NULL);
    PF->clear(&in);
    return res;
}

int dev_shm(int dev_pid, int fd, ewokos_addr_t node, uint8_t* contig, int* size) {
    proto_t in, out;
    PF->init(&out);
    PF->init(&in)->addi(&in, fd)->addi(&in, node);

    int32_t shm_id = -1;
    if(ipc_call(dev_pid, FS_CMD_SHM, &in, &out) == 0) {
        shm_id = proto_read_int(&out);
        *contig = proto_read_int(&out);
        if(size != NULL)
            *size = proto_read_int(&out);
    }
    PF->clear(&in);
    PF->clear(&out);
    return shm_id;
}

int dev_write_block(int pid, const void* buf, uint32_t size, int32_t index) {
    proto_t in, out;
    PF->init(&out);
    PF->format(&in, "m,i", buf, size, index);

    int res = -1;
    if(ipc_call(pid, FS_CMD_WRITE_BLOCK, &in, &out) == 0) {
        int r = proto_read_int(&out);
        res = r;
        if(res == -2) {
            errno = EAGAIN;
            res = -1;
        }
    }
    PF->clear(&in);
    PF->clear(&out);
    return res;
}

int dev_read_block(int pid, void* buf, uint32_t size, int32_t index) {
    int32_t shm_id = shmget(IPC_PRIVATE, size, 0666|IPC_CREAT);
    if(shm_id == -1) 
        return -1;
    void* shm = shmat(shm_id, 0, 0);
    if(shm == (void*)-1) {
        /* destroy the never-attached segment so it cannot leak */
        shmctl(shm_id, IPC_RMID, NULL);
        return 0;
    }

    proto_t in, out;
    PF->init(&out);
    PF->format(&in, "i,i,i", size, index, shm_id);

    int res = -1;
    if(ipc_call(pid, FS_CMD_READ_BLOCK, &in, &out) == 0) {
        int rd = proto_read_int(&out);
        res = rd;
        if(rd > 0) {
            memcpy(buf, shm, rd);
        }
        if(res == VFS_ERR_RETRY) {
            errno = EAGAIN;
            res = -1;
        }
    }
    PF->clear(&in);
    PF->clear(&out);
    shmdt(shm);
    return res;
}

int dev_cmd_cntl(const char* fname, int cmd, proto_t* in, proto_t* out) {
    return dev_cntl(fname, cmd, in, out);
}

#ifdef __cplusplus
}
#endif
