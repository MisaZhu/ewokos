#include <ewoksys/vdevice.h>
#include <ewoksys/vfs.h>
#include <ewoksys/ipc.h>
#include <ewoksys/ipc_serv.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <sys/shm.h>
#include <ewoksys/proc.h>
#include <ewoksys/syscall.h>
#include <ewoksys/signal.h>
#include <ewoksys/hashmap.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

static map_t  _files_hash = NULL;
/*
 * Serializes every access to _files_hash and the fsinfo_t entries it owns.
 * IPC_MULTI_TASK servers dispatch requests on concurrent kernel workers, so
 * the per-fd file cache must behave like a thread-safe table. The lock is a
 * leaf: never perform IPC (vfs_get_by_node etc.) while holding it.
 */
static pthread_mutex_t _files_lock;
static pthread_mutex_t _shm_lock;
static fsinfo_t* file_add(int fd, int pid, fsinfo_t* info);

static void device_init(vdevice_t* dev) {
    (void)dev;
    _files_hash = hashmap_new(0);
    pthread_mutex_init(&_files_lock, NULL);
    pthread_mutex_init(&_shm_lock, NULL);
}

#define FILE_KEY_LEN 64
static inline const char* file_hash_key(char* key, int fd, int pid, ewokos_addr_t node) {
    snprintf(key, FILE_KEY_LEN, "%x:%x:%llx", fd, pid,
            (unsigned long long)node);
    return key;
}

static inline int file_owner_pid(int pid) {
    return proc_getpid_or_raw(pid);
}

/* callers must hold _files_lock */
static fsinfo_t* file_get_cache(int fd, int pid, ewokos_addr_t node) {
    fsinfo_t* info = NULL;
    char key[FILE_KEY_LEN];
    hashmap_get(_files_hash, file_hash_key(key, fd, pid, node), (void**)&info);
    return info;
}

typedef struct {
    int pid;
    ewokos_addr_t node;
    fsinfo_t* info;
} file_clone_lookup_t;

static int file_find_same_owner_node(map_t in, const char* key, any_t value, any_t arg) {
    (void)in;
    fsinfo_t* info = (fsinfo_t*)value;
    file_clone_lookup_t* lookup = (file_clone_lookup_t*)arg;
    unsigned int fd_key = 0;
    unsigned int pid_key = 0;
    unsigned long long node_key = 0;

    if(info == NULL || lookup == NULL)
        return MAP_OK;
    if(sscanf(key, "%x:%x:%llx", &fd_key, &pid_key, &node_key) != 3)
        return MAP_OK;
    if((int)pid_key != lookup->pid || (ewokos_addr_t)node_key != lookup->node)
        return MAP_OK;

    lookup->info = info;
    return 1;
}

/* callers must hold _files_lock */
static fsinfo_t* file_clone_same_owner_node(int fd, int pid, ewokos_addr_t node) {
    file_clone_lookup_t lookup;
    lookup.pid = pid;
    lookup.node = node;
    lookup.info = NULL;
    hashmap_iterate(_files_hash, file_find_same_owner_node, &lookup);
    if(lookup.info == NULL)
        return NULL;
    return file_add(fd, pid, lookup.info);
}

/* callers must hold _files_lock */
static fsinfo_t* file_add(int fd, int pid, fsinfo_t* info) {
    char key_buf[FILE_KEY_LEN];
    pid = file_owner_pid(pid);
    const char* key = file_hash_key(key_buf, fd, pid, info->node);

    /* Reuse the entry when the same fd is registered again, otherwise the
     * previous fsinfo is dropped on the floor on every re-open. */
    fsinfo_t* ret = NULL;
    hashmap_get(_files_hash, key, (void**)&ret);
    if(ret == NULL) {
        ret = (fsinfo_t*)malloc(sizeof(fsinfo_t));
        if(ret == NULL)
            return NULL;
        if(hashmap_put(_files_hash, key, ret) != MAP_OK) {
            free(ret);
            return NULL;
        }
    }
    memcpy(ret, info, sizeof(fsinfo_t));
    return ret;
}

typedef struct {
        ewokos_addr_t node;
        int count;
} file_count_lookup_t;

static int file_count_same_node(map_t in, const char* key, any_t value, any_t arg) {
        (void)in;
        (void)key;
        fsinfo_t* info = (fsinfo_t*)value;
        file_count_lookup_t* lookup = (file_count_lookup_t*)arg;

        if(info == NULL || lookup == NULL)
                return MAP_OK;
        if(info->node == lookup->node)
                lookup->count++;
        return MAP_OK;
}

int vdevice_count_node_refs(ewokos_addr_t node) {
    file_count_lookup_t lookup;

    if(node == 0 || _files_hash == NULL)
        return 0;
    lookup.node = node;
    lookup.count = 0;
    pthread_mutex_lock(&_files_lock);
    hashmap_iterate(_files_hash, file_count_same_node, &lookup);
    pthread_mutex_unlock(&_files_lock);
    return lookup.count;
}

/* callers must hold _files_lock */
static void file_del(int fd, int pid, ewokos_addr_t node) {
    char key_buf[FILE_KEY_LEN];
    pid = file_owner_pid(pid);
    fsinfo_t* info = NULL;
    const char* key = file_hash_key(key_buf, fd, pid, node);
    hashmap_get(_files_hash, key, (void**)&info);
    if(info == NULL)
        return;

    hashmap_remove(_files_hash, key);
    free(info);
}

/*
 * Copy-out lookup used by the request handlers: fills the caller's stack
 * fsinfo_t so a concurrent do_close()/file_del() on another worker thread
 * can never free the entry from under this handler. The vfsd lookup
 * fallback runs outside the lock (it is an IPC round-trip).
 *
 * The server-side entry is authoritative: it was created by do_open() and
 * kept in sync by dev_update_file(). Do NOT overwrite it with the
 * client-supplied blob — for anonymous single-node devices (e.g. /dev/net0)
 * a client blob can hold a sibling fd's private state, which would poison
 * this entry and make close/read/write operate on the wrong instance.
 */
static int dev_fetch_file_seeded(int fd, int pid, ewokos_addr_t node, const fsinfo_t* seed, fsinfo_t* out) {
    pid = file_owner_pid(pid);

    pthread_mutex_lock(&_files_lock);
    fsinfo_t* info = file_get_cache(fd, pid, node);
    if(info == NULL && seed != NULL)
        info = file_add(fd, pid, (fsinfo_t*)seed);
    if(info == NULL)
        info = file_clone_same_owner_node(fd, pid, node);
    if(info != NULL) {
        memcpy(out, info, sizeof(fsinfo_t));
        pthread_mutex_unlock(&_files_lock);
        return 0;
    }
    pthread_mutex_unlock(&_files_lock);

    fsinfo_t i;
    if(vfs_get_by_node(node, &i) != 0)
        return -1;

    pthread_mutex_lock(&_files_lock);
    info = file_get_cache(fd, pid, node); /* re-check: raced with another worker */
    if(info == NULL)
        info = file_add(fd, pid, &i);
    if(info == NULL) {
        pthread_mutex_unlock(&_files_lock);
        return -1;
    }
    memcpy(out, info, sizeof(fsinfo_t));
    pthread_mutex_unlock(&_files_lock);
    return 0;
}

static int dev_fetch_file(int fd, int pid, ewokos_addr_t node, fsinfo_t* out) {
    return dev_fetch_file_seeded(fd, pid, node, NULL, out);
}

/*
 * Legacy pointer API kept for single-task drivers; the returned entry is
 * only stable while no concurrent close can drop it. Multi-task request
 * handlers use the dev_fetch_file*() copy variants instead.
 */
fsinfo_t* dev_get_file(int fd, int pid, ewokos_addr_t node) {
    pid = file_owner_pid(pid);

    pthread_mutex_lock(&_files_lock);
    fsinfo_t* info = file_get_cache(fd, pid, node);
    if(info == NULL)
        info = file_clone_same_owner_node(fd, pid, node);
    pthread_mutex_unlock(&_files_lock);
    if(info != NULL)
        return info;

    fsinfo_t i;
    if(vfs_get_by_node(node, &i) != 0)
        return NULL;

    pthread_mutex_lock(&_files_lock);
    info = file_get_cache(fd, pid, node);
    if(info == NULL)
        info = file_add(fd, pid, &i);
    pthread_mutex_unlock(&_files_lock);
    return info;
}

int dev_update_file(int fd, int from_pid, fsinfo_t* finfo) {
    if(finfo == NULL)
        return -1;
    /* insert-or-overwrite: net effect matches the old lookup+memcpy */
    pthread_mutex_lock(&_files_lock);
    fsinfo_t* info = file_add(fd, from_pid, finfo);
    pthread_mutex_unlock(&_files_lock);
    return (info == NULL) ? -1 : 0;
}

static void do_open(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
    int oflag;
    int fd = proto_read_int(in);
    ewokos_addr_t node = proto_read_int(in);
    int32_t caller_info_size = 0;
    fsinfo_t* caller_info = (fsinfo_t*)proto_read(in, &caller_info_size);
    oflag = proto_read_int(in);

    if(caller_info != NULL &&
            (caller_info_size != (int32_t)sizeof(fsinfo_t) || caller_info->node != node))
        caller_info = NULL;

    fsinfo_t info;
    if(vfs_get_by_node(node, &info) != 0) {
        /* The caller already owns the fsinfo returned by VFS_OPEN, so trust it
         * rather than failing the open when the vfsd lookup does not answer. */
        if(caller_info == NULL) {
            PF->addi(out, -1)->addi(out, ENOENT);
            return;
        }
        memcpy(&info, caller_info, sizeof(fsinfo_t));
    }

    if((oflag & O_WRONLY) != 0 && vfs_check_access(from_pid, &info, W_OK) != 0) {
        PF->addi(out, -1)->addi(out, EPERM);
        return;
    }

    /*
     * Mirror VFS open permission semantics: pure write-only opens must not be
     * rejected just because the caller lacks read permission.
     */
    if((oflag & O_WRONLY) == 0 && vfs_check_access(from_pid, &info, R_OK) != 0) {
        PF->addi(out, -1)->addi(out, EPERM);
        return;
    }
    
    int res = 0;
    if(fd >= 0 && dev != NULL && dev->open != NULL) {
        if(dev->open(dev, fd, from_pid, &info, oflag, p) != 0)
            res = -1;
    }
    PF->addi(out, res);
    if(res == 0) {
        pthread_mutex_lock(&_files_lock);
        file_add(fd, from_pid, &info);
        pthread_mutex_unlock(&_files_lock);
        PF->add(out, &info, sizeof(fsinfo_t));
    }
    else
        PF->addi(out, errno);
}

static void do_close(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
    //all close ipc are from vfsd proc, so read owner pid for real owner.
    (void)out;
    int fd = proto_read_int(in);
    ewokos_addr_t node = proto_read_int(in);
    fsinfo_t* fsinfo = proto_read(in, NULL); //blob only advances the cursor, never trusted below
    int close_pid = proto_read_int(in);
    int owner_pid = proto_read_int(in);
    if(close_pid > 0) {
        int vfsd_pid = get_vfsd_pid(); //from vfsd for proc exit closing.
        if(vfsd_pid == from_pid) {
            from_pid = close_pid;
        }
    }
    if(owner_pid <= 0)
        owner_pid = from_pid;
    owner_pid = file_owner_pid(owner_pid);
    (void)fsinfo;

    /*
     * Only the device-side per-fd entry is authoritative for close: it was
     * created by do_open()/do_dup() and kept in sync by dev_update_file().
     * A close must NEVER fall back to node-level resurrection
     * (dev_fetch_file()'s vfs_get_by_node path): a stale/duplicate
     * FS_CMD_CLOSE arrives after file_del() already dropped the entry (the
     * fire-and-forget VFS_CLOSE races with vfsd's zombie cleanup, which can
     * re-ship a close for the same fd), and resurrecting the entry would run
     * dev->close() twice for one descriptor, double-decrementing the
     * driver's refcount (piped destroyed the pipe early; netd dropped a live
     * connection). No entry means this server holds no state for the fd, so
     * skip the device close entirely: there is nothing to release.
     *
     * Lookup and delete are one locked section so that two closes racing on
     * the same (fd, owner, node) claim the entry exactly once.
     */
    fsinfo_t finfo;
    bool has_info = false;
    pthread_mutex_lock(&_files_lock);
    fsinfo_t* info = file_get_cache(fd, owner_pid, node);
    if(info != NULL) {
        memcpy(&finfo, info, sizeof(fsinfo_t));
        has_info = true;
        file_del(fd, owner_pid, node);
    }
    pthread_mutex_unlock(&_files_lock);

    if(has_info && dev != NULL && dev->close != NULL) {
        dev->close(dev, fd, owner_pid, node, &finfo, p);
    }
}

static void do_dup(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
    (void)out;
    int from_fd = proto_read_int(in);
    int dup_fd = proto_read_int(in);
    ewokos_addr_t node = proto_read_int(in);
    fsinfo_t* fsinfo = proto_read(in, NULL);
    int from_owner_pid = proto_read_int(in);
    int dup_owner_pid = proto_read_int(in);

    if(dev != NULL && dev->dup != NULL) {
        dev->dup(dev, from_fd, from_owner_pid, dup_fd, dup_owner_pid, node, fsinfo, p);
    }
    /*
     * Keep the device-side per-fd cache aligned with VFS dup/dup2/fork
     * semantics. Anonymous devices like /dev/net0 carry fd-private data in
     * fsinfo.data, so falling back to node-level lookup on the first access of
     * the duplicated fd would lose the socket instance binding.
     */
    pthread_mutex_lock(&_files_lock);
    file_del(dup_fd, dup_owner_pid, node);
    if(fsinfo != NULL) {
        file_add(dup_fd, dup_owner_pid, fsinfo);
    }
    pthread_mutex_unlock(&_files_lock);
    (void)from_pid;
}

#define READ_BUF_SIZE 32

/*
 * Server-side shm mapping cache.
 *
 * The client (devcmd.c dev_read/dev_write) now reuses a stable shm_id per fd,
 * so instead of shmat()+shmdt() on every read/write - each a page map/unmap
 * with a global TLB flush in the kernel - we map a segment once and keep it.
 * A small LRU bounds the number of live mappings; evicting shmdt()s the
 * segment, which also releases it in the kernel once the client has detached.
 * We hold a mapping ref while cached, so a cached addr always stays valid.
 *
 * Only do_read/do_write use this: do_read_block's client still allocates an
 * ephemeral IPC_PRIVATE segment per call (new id each time), so caching there
 * would never hit and would just pin dead segments.
 */
#define SHM_CACHE_NUM 16
typedef struct {
    int32_t  shm_id;
    void*    addr;
    uint32_t lru;   /* higher = more recently used */
    int32_t  pins;  /* concurrent users; pinned entries are never evicted */
} shm_cache_t;
static shm_cache_t _shm_cache[SHM_CACHE_NUM];
static uint32_t _shm_cache_tick = 0;

static void* shm_cache_get(int32_t shm_id) {
    if(shm_id <= 0)
        return NULL;

    pthread_mutex_lock(&_shm_lock);
    for(int i=0; i<SHM_CACHE_NUM; i++) {
        if(_shm_cache[i].addr != NULL && _shm_cache[i].shm_id == shm_id) {
            _shm_cache[i].lru = ++_shm_cache_tick;
            _shm_cache[i].pins++;
            void* cached = _shm_cache[i].addr;
            pthread_mutex_unlock(&_shm_lock);
            return cached;
        }
    }
    pthread_mutex_unlock(&_shm_lock);

    /* map outside the lock: shmat is a kernel round-trip */
    void* addr = shmat(shm_id, 0, 0);
    if(addr == (void*)-1 || addr == NULL)
        return NULL;

    pthread_mutex_lock(&_shm_lock);
    int free_i = -1, lru_i = -1;
    uint32_t lru_min = 0xffffffff;
    for(int i=0; i<SHM_CACHE_NUM; i++) {
        if(_shm_cache[i].addr != NULL && _shm_cache[i].shm_id == shm_id) {
            /* another worker cached the same segment while we were mapping */
            _shm_cache[i].lru = ++_shm_cache_tick;
            _shm_cache[i].pins++;
            void* cached = _shm_cache[i].addr;
            pthread_mutex_unlock(&_shm_lock);
            shmdt(addr);
            return cached;
        }
        if(_shm_cache[i].addr == NULL) {
            if(free_i < 0)
                free_i = i;
        }
        else if(_shm_cache[i].pins == 0 && _shm_cache[i].lru < lru_min) {
            lru_min = _shm_cache[i].lru;
            lru_i = i;
        }
    }

    int slot = (free_i >= 0) ? free_i : lru_i;
    if(slot < 0) {
        /* every slot pinned by an in-flight request: hand out a one-shot
         * mapping; shm_cache_put() detaches it */
        pthread_mutex_unlock(&_shm_lock);
        return addr;
    }
    void* evict = _shm_cache[slot].addr;
    _shm_cache[slot].shm_id = shm_id;
    _shm_cache[slot].addr = addr;
    _shm_cache[slot].lru = ++_shm_cache_tick;
    _shm_cache[slot].pins = 1;
    pthread_mutex_unlock(&_shm_lock);
    if(evict != NULL)
        shmdt(evict); /* evict unpinned LRU */
    return addr;
}

/* release a mapping obtained via shm_cache_get() */
static void shm_cache_put(int32_t shm_id, void* addr) {
    if(shm_id <= 0 || addr == NULL || addr == (void*)-1)
        return;
    pthread_mutex_lock(&_shm_lock);
    for(int i=0; i<SHM_CACHE_NUM; i++) {
        if(_shm_cache[i].addr == addr && _shm_cache[i].shm_id == shm_id) {
            if(_shm_cache[i].pins > 0)
                _shm_cache[i].pins--;
            pthread_mutex_unlock(&_shm_lock);
            return;
        }
    }
    pthread_mutex_unlock(&_shm_lock);
    shmdt(addr); /* one-shot mapping that never entered the cache */
}

static void do_read(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
    int size, offset;
    int fd = proto_read_int(in);
    ewokos_addr_t node = proto_read_int(in);
    size = proto_read_int(in);
    offset = proto_read_int(in);
    int32_t shm_id = proto_read_int(in);
    fsinfo_t seed_info, finfo;
    char buffer[READ_BUF_SIZE];
    int32_t rd = -1;

    proto_read_to(in, &seed_info, sizeof(fsinfo_t));
    if(dev_fetch_file_seeded(fd, from_pid, node, &seed_info, &finfo) != 0) {
        PF->addi(out, -1);
        return;
    }
    /*if(vfs_check_access(from_pid, info, R_OK) != 0) {
        PF->addi(out, -1)->addi(out, EPERM);
        return;
    }
    */

    if(dev != NULL && dev->read != NULL) {
        void* buf;
        if(shm_id == -1) {
            if(size > READ_BUF_SIZE)
                buf = malloc(size);
            else
                buf = buffer;
        }
        else {
            buf = shm_cache_get(shm_id);
        }

        if(buf == (void*)-1 || buf == NULL) {
            PF->addi(out, -1);
        }
        else {
            rd = dev->read(dev, fd, from_pid, &finfo, buf, size, offset, p);
            PF->addi(out, rd);
            if(rd > 0) {
                if(rd > size)
                    rd = size;
                if(shm_id == -1) {
                    PF->add(out, buf, rd);
                }
            }
            else if(rd < 0) {
                PF->addi(out, errno);
            }

            /* cached shm stays mapped; only free the malloc fallback */
            if(shm_id == -1 && buf != buffer)
                free(buf);
        }
        if(shm_id != -1)
            shm_cache_put(shm_id, buf);
    }
    else {
        PF->addi(out, -1);
    }

    (void)rd;
    /*
     * Readers like netd/task_read() now clear stale RD before arming a fresh
     * async receive. Clearing RD again here after a RETRY reintroduces the same
     * race we fixed on the write side: a new readable edge can arrive between
     * check_poll_events() and this userspace-side clear, so the blocked reader
     * goes to sleep after the only wakeup for the newly arrived byte stream was
     * just erased.
     */
}

static void do_write(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
    int32_t size, offset;
    int fd = proto_read_int(in);
    ewokos_addr_t node = proto_read_int(in);
    offset = proto_read_int(in);
    int32_t shm_id = proto_read_int(in);
    fsinfo_t seed_info, finfo;
    int32_t wr = -1;
    
    proto_read_to(in, &seed_info, sizeof(fsinfo_t));
    if(dev_fetch_file_seeded(fd, from_pid, node, &seed_info, &finfo) != 0) {
        PF->addi(out, -1);
        return;
    }
    /*if(vfs_check_access(from_pid, info, W_OK) != 0) {
        PF->addi(out, -1)->addi(out, EPERM);
        return;
    }
    */
    
    if(dev != NULL && dev->write != NULL) {
        void* data;
        if(shm_id == -1)
            data = proto_read(in, &size);
        else {
            size = proto_read_int(in);
            data = shm_cache_get(shm_id);
        }

        if(data == (void*)-1 || data == NULL) {
            PF->addi(out, -1);
        }
        else {
            size = dev->write(dev, fd, from_pid, &finfo, data, size, offset, p);
            wr = size;
            finfo.state |= FS_STATE_CHANGED;
            dev_update_file(fd, from_pid, &finfo);
            PF->addi(out, size);
            PF->add(out, &finfo, sizeof(fsinfo_t));
            if(size < 0) {
                PF->addi(out, errno);
            }
        }
        /* cached shm stays mapped; proto_read path owns no buffer to free */
        if(shm_id != -1)
            shm_cache_put(shm_id, data);
    }
    else {
        PF->addi(out, -1);
    }

    (void)wr;
    /*
     * Writers like netd clear stale WR edges when arming a new async send.
     * Clearing again here after the device write returns RETRY races against a
     * fast completion: the worker can finish and publish a fresh WR wakeup
     * between check_poll_events() and this userspace-side clear, losing the
     * only edge the blocked writer was waiting for.
     */
}

static void do_read_block(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
    int size, index;
    size = proto_read_int(in);
    index = proto_read_int(in);
    int32_t shm_id = proto_read_int(in);

    if(dev != NULL && dev->read_block != NULL) {
        void* buf;
        if(shm_id == -1)
            buf = malloc(size);
        else
            buf = shmat(shm_id, 0, 0);
        if(buf == (void*)-1 || buf == NULL) {
            PF->addi(out, -1);
        }
        else {
            size = dev->read_block(dev, from_pid, buf, size, index, p);
            PF->addi(out, size);
            if(size > 0) {
                if(shm_id == -1) {
                    PF->add(out, buf, size);
                }
            }

            if(shm_id != -1 && buf != NULL)
                shmdt(buf);
            else
                free(buf);
        }
    }
    else {
        PF->addi(out, -1);
    }
}

static void do_write_block(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
    int32_t size, index;
    void* data = proto_read(in, &size);
    index = proto_read_int(in);

    if(dev != NULL && dev->write_block != NULL) {
        size = dev->write_block(dev, from_pid, data, size, index, p);
        PF->addi(out, size);
    }
    else {
        PF->addi(out, -1);
    }
}

static void do_shm(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
    int fd = proto_read_int(in);
    ewokos_addr_t node = proto_read_int(in);

    int shm_id = -1;	
    int size = 0;
    uint8_t contig = 0;
    if(dev != NULL && dev->shm != NULL) {
        fsinfo_t finfo;
        if(dev_fetch_file(fd, from_pid, node, &finfo) == 0)
            shm_id = dev->shm(dev, fd, from_pid, &finfo, &contig, &size, p);
    }
    PF->addi(out, shm_id)->addi(out, contig)->addi(out, size);
}

static void do_fcntl(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
    int fd = proto_read_int(in);
    ewokos_addr_t node = proto_read_int(in);
    int32_t cmd = proto_read_int(in);
    fsinfo_t seed_info, finfo;

    proto_read_to(in, &seed_info, sizeof(fsinfo_t));
    if(dev_fetch_file_seeded(fd, from_pid, node, &seed_info, &finfo) != 0) {
        PF->addi(out, -1);
        return;
    }

    proto_t arg_in, arg_out;
    PF->init(&arg_out);

    int32_t arg_size;
    void* arg_data = proto_read(in, &arg_size);
    PF->init_data(&arg_in, arg_data, arg_size);

    int res = -1;
    if(dev != NULL && dev->fcntl != NULL) {
        res = dev->fcntl(dev, fd, from_pid, &finfo, cmd, &arg_in, &arg_out, p);
        if(res == 0)
            dev_update_file(fd, from_pid, &finfo);
    }
    PF->clear(&arg_in);

    PF->addi(out, res)->
            add(out, &finfo, sizeof(fsinfo_t))->
            add(out, arg_out.data, arg_out.size);
    PF->clear(&arg_out);
}

static void do_flush(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
    (void)from_pid;
    int fd = proto_read_int(in);
    ewokos_addr_t node = proto_read_int(in);
    fsinfo_t finfo;

    if(dev_fetch_file(fd, from_pid, node, &finfo) != 0) {
        PF->addi(out, -1)->addi(out, ENOENT);
        return;
    }
    /*if(vfs_check_access(from_pid, info, W_OK) != 0) {
        PF->addi(out, -1)->addi(out, EPERM);
        return;
    }
    */

    int ret = 0;
    if(dev != NULL && dev->flush != NULL) {
        ret = dev->flush(dev, fd, from_pid, &finfo, p);
    }
    PF->addi(out, ret);
}

static void do_create(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
    (void)from_pid;
    ewokos_addr_t node_to = proto_read_int(in);
    ewokos_addr_t node = proto_read_int(in);
    fsinfo_t info_to, info;

    if(vfs_get_by_node(node_to, &info_to) != 0) {
        PF->addi(out, -1)->addi(out, ENOENT);
        return;
    }

    if(vfs_get_by_node(node, &info) != 0) {
        PF->addi(out, -1)->addi(out, ENOENT);
        return;
    }

    if(vfs_check_access(from_pid, &info_to, W_OK) != 0) {
        PF->addi(out, -1)->addi(out, EPERM);
        return;
    }

    int res = 0;
    if(dev != NULL && dev->create != NULL)
        res = dev->create(dev, from_pid, &info_to, &info, p);

    if(res == 0) {
        PF->addi(out, res)->add(out, &info, sizeof(fsinfo_t));
        return;
    }		
    PF->addi(out, -1);
}

static void do_unlink(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
    (void)from_pid;
    ewokos_addr_t node = proto_read_int(in);
    const char* fname = proto_read_str(in);

    fsinfo_t info;
    if(vfs_get_by_node(node, &info) != 0) {
        PF->addi(out, -1)->addi(out, ENOENT);
        return;
    }
    
    if(vfs_check_access(from_pid, &info, W_OK) != 0) {
        PF->addi(out, -1)->addi(out, EPERM);
        return;
    }

    int res = 0;
    if(dev != NULL && dev->unlink != NULL)
        res = dev->unlink(dev, &info, fname, p);
    else if(!FS_IS_TYPE(info.type, FS_TYPE_FILE) &&
            !FS_IS_TYPE(info.type, FS_TYPE_DIR)) {
        PF->addi(out, -1)->addi(out, EPERM);
        return;
    }
    else
        res = vfs_del_node(info.node);
    PF->addi(out, res);
}

static void do_set(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
    fsinfo_t info_old, info;
    proto_read_to(in, &info, sizeof(fsinfo_t));
    if(vfs_get_by_node(info.node, &info_old) != 0) {
        PF->addi(out, -1)->addi(out, ENOENT);
        return;
    }
    
    if(vfs_check_access(from_pid, &info_old, W_OK) != 0) {
        PF->addi(out, -1)->addi(out, EPERM);
        return;
    }

    int res = 0;
    if(dev != NULL && dev->set != NULL)
        res = dev->set(dev, from_pid, &info, p);
    PF->addi(out, res);
}

static void do_get(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
    fsinfo_t info;
    const char* fname = proto_read_str(in);

    int res = -1;
    if(dev != NULL && dev->get != NULL)
        res = dev->get(dev, from_pid, fname, &info, p);
    if(res == 0)
        PF->addi(out, res)->add(out, &info, sizeof(fsinfo_t));
    else
        PF->addi(out, res);
}

static void do_kids(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
    (void)from_pid;
    ewokos_addr_t node = proto_read_int(in);
    fsinfo_t info;
    proto_read_to(in, &info, sizeof(fsinfo_t));

    if(vfs_check_access(from_pid, &info, R_OK) != 0) {
        klog("error dev_kids: %d\n", info.node);
        PF->addi(out, -1)->addi(out, EPERM);
        return;
    }

    fsinfo_t* kids = NULL;
    uint32_t num;
    if(dev != NULL && dev->kids != NULL)
        kids = dev->kids(dev, &info, &num, p);
    if(kids == NULL || num == 0)
        return;
    PF->clear(out)->addi(out, num)->add(out, kids, sizeof(fsinfo_t)*num);
}

static void do_stat(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
    fsinfo_t info;
    proto_read_to(in, &info, sizeof(fsinfo_t));

    node_stat_t stat;

    if(dev != NULL && dev->stat != NULL) {
        int ret = dev->stat(dev, from_pid, &info, &stat, p);
        PF->addi(out, ret);
        PF->add(out, &stat, sizeof(node_stat_t));
    }else{
        PF->addi(out, -1);
    }
}

static char* read_cmd_arg(char* cmd, int* offset) {
    char* p = NULL;
    uint8_t quotes = 0;

    while(cmd[*offset] != 0) {
        char c = cmd[*offset];
        (*offset)++;
        if(quotes) { //read whole quotes content.
            if(c == '"') {
                cmd[*offset-1] = 0;
                return p;
            }
            continue;
        }
        if(c == ' ') { //read next arg.
            if(p == NULL) //skip begin spaces.
                continue;
            cmd[*offset-1] = 0;
            break;
        }
        else if(p == NULL) {
            if(c == '"') { //if start of quotes.
                quotes = 1;
                (*offset)++;
            }
            p = cmd + *offset - 1;
        }
    }
    return p;
}

#define ARG_MAX 16

static char* gen_str(const char* s) {
    char* res;
    if(s == NULL)
        return NULL;
    res = (char*)malloc(strlen(s)+1);
    if(res == NULL)
        return NULL;
    strcpy(res, s);
    return res;
}

static char* do_basic_cmd(vdevice_t* dev, int argc, char** argv) {
    if(strcmp(argv[0], "dev.echo") == 0) {
        if(argc > 1)
            return gen_str(argv[1]);
        else
            return gen_str("");
    }
    return NULL;
}

static void do_cmd(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
    const char* cmd = proto_read_str(in);
    if(cmd == NULL || cmd[0] == 0)
        return;

    char* argv[ARG_MAX] = {0};
    int argc = 0;
    int offset = 0;

    while(argc < ARG_MAX) {
        char* arg = read_cmd_arg((char*)cmd, &offset); 
        if(arg == NULL || arg[0] == 0)
            break;
        argv[argc] = (char*)malloc(strlen(arg)+1);
        if(argv[argc] == NULL)
            break;
        strcpy(argv[argc], arg);
        argc++;
    }

    if(argc == 0)
        return;

    char* res = do_basic_cmd(dev, argc, argv);
    if(res == NULL && dev != NULL && dev->cmd != NULL)
        res = dev->cmd(dev, from_pid, argc, argv, p);

    argc = 0;
    while(argc < ARG_MAX) {
        if(argv[argc] != NULL)
            free(argv[argc]);
        argc++;
    }

    if(res != NULL) {
        PF->adds(out, res);
        free(res);
    }
}

static void do_clear_buffer(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
    (void)from_pid;
    ewokos_addr_t node = proto_read_int(in);

    int res = -1;
    if(dev != NULL && dev->clear_buffer != NULL)
        res = dev->clear_buffer(dev, node, p);
    PF->addi(out, res);
}

static void do_dev_cntl(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
    PF->addi(out, -1);
    if(dev == NULL || dev->dev_cntl == NULL)
        return;

    int cmd = proto_read_int(in);
    int32_t  sz;
    void* data = proto_read(in, &sz);
    
    proto_t in_arg, ret;
    PF->init(&in_arg);
    PF->init(&ret);
    if(data != NULL && sz > 0) 
        PF->copy(&in_arg, data, sz);

    int32_t res = dev->dev_cntl(dev, from_pid, cmd, &in_arg, &ret, p);
    if(res == 0) {
        PF->clear(out)->addi(out, 0)->add(out, ret.data, ret.size);
    }
    else if(res < 0) {
        /*
         * Report the handler's own failure code rather than a flat -1, so a
         * driver can tell the caller which stage of the operation failed.
         * No reply payload is attached: the caller only reads one on success.
         */
        PF->clear(out)->addi(out, res);
    }
    PF->clear(&in_arg);
    PF->clear(&ret);
}

static void do_poll(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
    int fd = proto_read_int(in);
    ewokos_addr_t node = proto_read_int(in);
    fsinfo_t seed_info, finfo;

    PF->addi(out, -1);
    proto_read_to(in, &seed_info, sizeof(fsinfo_t));
    if(dev_fetch_file_seeded(fd, from_pid, node, &seed_info, &finfo) != 0 ||
            dev == NULL || dev->check_poll_events == NULL) {
        return;
    }

    uint32_t events = dev->check_poll_events(dev, fd, from_pid, &finfo, p);
    PF->clear(out)->addi(out, 0)->addi(out, events);
}

static void do_interrupt(vdevice_t* dev, proto_t *in, void* p) {
    if(dev != NULL && dev->interrupt != NULL) {
        dev->interrupt(dev, in, p);
    }
}

static void handle(int from_pid, int cmd, proto_t* in, proto_t* out, void* p) {
    vdevice_t* dev = (vdevice_t*)p;
    if(dev == NULL)
        return;
    PF->clear(out);

    p = dev->extra_data;
    switch(cmd) {
    case FS_CMD_OPEN:
        do_open(dev, from_pid, in, out, p);
        break;
    case FS_CMD_STAT:
        do_stat(dev, from_pid, in, out, p);
        break;
    case FS_CMD_CLOSE:
        do_close(dev, from_pid, in, out, p);
        break;
    case FS_CMD_DUP:
        do_dup(dev, from_pid, in, out, p);
        break;
    case FS_CMD_READ:
        do_read(dev, from_pid, in, out, p);
        break;
    case FS_CMD_WRITE:
        do_write(dev, from_pid, in, out, p);
        break;
    case FS_CMD_READ_BLOCK:
        do_read_block(dev, from_pid, in, out, p);
        break;
    case FS_CMD_WRITE_BLOCK:
        do_write_block(dev, from_pid, in, out, p);
        break;
    case FS_CMD_SHM:
        do_shm(dev, from_pid, in, out, p);
        break;
    case FS_CMD_FLUSH:
        do_flush(dev, from_pid, in, out, p);
        break;
    case FS_CMD_CNTL:
        do_fcntl(dev, from_pid, in, out, p);
        break;
    case FS_CMD_CREATE:
        do_create(dev, from_pid, in, out, p);
        break;
    case FS_CMD_UNLINK:
        do_unlink(dev, from_pid, in, out, p);
        break;
    case FS_CMD_SET:
        do_set(dev, from_pid, in, out, p);
        break;
    case FS_CMD_GET:
        do_get(dev, from_pid, in, out, p);
        break;
    case FS_CMD_KIDS:
        do_kids(dev, from_pid, in, out, p);
        break;
    case FS_CMD_CMD:
        do_cmd(dev, from_pid, in, out, p);
        break;
    case FS_CMD_POLL:
        do_poll(dev, from_pid, in, out, p);
        break;
    case FS_CMD_CLEAR_BUFFER:
        do_clear_buffer(dev, from_pid, in, out, p);
        break;
    case FS_CMD_INTERRUPT:
        do_interrupt(dev, in, p);
        break;
    case FS_CMD_DEV_CNTL:
        do_dev_cntl(dev, from_pid, in, out, p);
        break;
    }
}

static int do_mount(vdevice_t* dev, int type, int mode) {
    fsinfo_t info;
    memset(&info, 0, sizeof(fsinfo_t));

    //create a non-father node 
    strcpy(info.name, dev->mnt_info.name);
    info.type = type;
    if(FS_IS_TYPE(type, FS_TYPE_DIR))
        info.stat.size = 1024;
    info.stat.uid = getuid();
    info.stat.gid = getgid();
    info.stat.mode = mode;
    vfs_new_node(&info, 0, true, false); // 0 means no father node

    if(dev->mount != NULL) { //do device mount precess
        if(dev->mount(dev, &info, dev->extra_data) != 0) {
            vfs_del_node(info.node);
            return -1;
        }
    }

    //mount the new node to mnt_point, previous node will be saved as well
    if(vfs_mount(dev->mnt_info.node, info.node, dev->desc) != 0) {
        vfs_del_node(info.node);
        return -1;
    }
    memcpy(&dev->mnt_info, &info, sizeof(fsinfo_t));
    return 0;
}

static void sig_stop(int sig_no, void* p) {
  (void)sig_no;
  vdevice_t* dev = (vdevice_t*)p;
  dev->terminated = true;
}

static void device_handled(void* p) {
    vdevice_t* dev = (vdevice_t*)p;
    if (dev != NULL && dev->handled != NULL) {
        dev->handled(dev, dev->extra_data);
    }
}

static void* device_loop_thread_entry(void* arg) {
        vdevice_t* dev = (vdevice_t*)arg;

        if(dev == NULL)
                return NULL;

        while(!dev->terminated) {
                if(dev->loop_step != NULL)
                        dev->loop_step(dev, dev->extra_data);
                else
                        usleep(100000);
        }
        return NULL;
}

void device_stop(vdevice_t* dev) {
    if(dev == NULL)
        return;

    dev->terminated = true;
}

int dev_cntl_by_pid(int pid, int cmd, proto_t* in, proto_t* out) {
    proto_t in_arg;
    PF->init(&in_arg)->addi(&in_arg, cmd);

    if(in != NULL)
        PF->add(&in_arg, in->data, in->size);

    int res = -1;
    if(out != NULL) {
        proto_t ret;
        PF->init(&ret);
        res = ipc_call(pid, FS_CMD_DEV_CNTL, &in_arg, &ret);
        PF->clear(&in_arg);
        if(res != 0) {
            PF->clear(&ret);
            return -1;
        }

        res = proto_read_int(&ret);
        /*
         * Negative handler status is passed through unchanged: drivers use
         * distinct negative codes to report which stage of an operation
         * failed, and callers test against zero rather than against -1.
         * Anything positive is not a valid status, so it is normalised.
         */
        if(res > 0)
            res = -1;
        if(res == 0) {
            int32_t sz;
            void *data = proto_read(&ret, &sz);
            PF->copy(out, data, sz);
        }
        PF->clear(&ret);
    }
    else {
        res = ipc_call(pid, FS_CMD_DEV_CNTL, &in_arg, NULL);
        PF->clear(&in_arg);
    }
    return res;
}

char* dev_cmd_by_pid(int pid, const char* cmd) {
    proto_t in_arg;
    PF->init(&in_arg)->adds(&in_arg, cmd);

    int res = -1;
    proto_t out;
    PF->init(&out);
    res = ipc_call(pid, FS_CMD_CMD, &in_arg, &out);
    PF->clear(&in_arg);
    if(res != 0) {
        PF->clear(&out);
        return NULL;
    }

    const char* s = proto_read_str(&out);
    char* ret = NULL;
    if(s != NULL) {
        ret = (char*)calloc(1, strlen(s)+1);
        strcpy(ret, s);
    }
    PF->clear(&out);
    return ret;
}

int dev_get_pid(const char* fname) {
    fsinfo_t info;
    if(vfs_get_by_name(fname, &info) != 0)
        return -1;
    return info.mount_pid;
}

int dev_cntl(const char* fname, int cmd, proto_t* in, proto_t* out) {
    int pid = dev_get_pid(fname);
    if(pid < 0)
        return -1;
    return dev_cntl_by_pid(pid, cmd, in, out);
}

char* dev_cmd(const char* fname, const char* cmd) {
    int pid = dev_get_pid(fname);
    if(pid < 0)
        return NULL;
    return dev_cmd_by_pid(pid, cmd);
}

int device_run(vdevice_t* dev, const char* mnt_point, int mnt_type, int mode, bool multi_task) {
    if(dev == NULL)
        return -1;
    device_init(dev);

    sys_signal(SYS_SIG_STOP, sig_stop, dev);

    if(mnt_point != NULL) {
        if(vfs_get_by_name(mnt_point, &dev->mnt_info) != 0) {
            if(vfs_create(mnt_point, &dev->mnt_info, FS_BASE_TYPE(mnt_type), mode, true, true) != 0)
                return -1;
        }

        if(do_mount(dev, mnt_type, mode) != 0)
            return -1;
    }

    int ipc_flags = 0;
    pthread_t loop_tid;
    bool loop_thread_started = false;

    if(dev->loop_step != NULL)
        ipc_flags |= IPC_NON_BLOCK;

    if(multi_task) {
        /*
         * IPC_MULTI_TASK dispatches every request on a concurrent kernel
         * worker task that shares THIS process's heap with the main context
         * and with every other worker. newlib's malloc/free are only safe
         * under that concurrency when __malloc_lock()/__malloc_unlock()
         * actually take the guard, and those are gated on
         * _proc_global_need_lock -- a flag otherwise flipped on solely by
         * thread_create(). A multi-task server that spawns no libc thread of
         * its own (netd leaves loop_step_threaded false and lets the kernel
         * pool run the handlers, so the pthread_create() below never fires)
         * would leave the heap UNPROTECTED: the main context and pool workers
         * malloc/free proto_t and I/O buffers concurrently, corrupting the
         * free list and wedging a worker inside the allocator. The stuck
         * worker never replies, so the client's IPC (e.g. a socket recv)
         * hangs forever even though the driver already holds the data -- the
         * xBrowser stall right after a large HTTP response. Arm the guard
         * while still single-threaded, BEFORE ipc_serv_run() opens the pool.
         */
        proc_malloc_lock_prepare();
        _proc_global_need_lock = true;
        ipc_flags |= IPC_MULTI_TASK;
    }

    ipc_serv_run(handle, device_handled, dev, ipc_flags);

    if(dev->loop_step != NULL && dev->loop_step_threaded) {
        if(pthread_create(&loop_tid, NULL, device_loop_thread_entry, dev) == 0) {
            pthread_detach(loop_tid);
            loop_thread_started = true;
        }
    }

    while(!dev->terminated) {
        if(loop_thread_started) {
            usleep(100000);
        }
        else if(dev->loop_step != NULL) {
            dev->loop_step(dev, dev->extra_data);
        }
        else {
            usleep(100000);
        }
    }

    /*
     * Stop accepting new IPC requests before tearing down the mount/cache.
     * Otherwise late FS_CMD_CLOSE / POLL traffic can race with userspace
     * cleanup and strand the service in teardown even though the caller app
     * has already decided to exit.
     */
    ipc_disable();
    if(mnt_point != NULL && dev->umount != NULL) {
        dev->umount(dev, dev->mnt_info.node, dev->extra_data);
    }
    vfs_umount(dev->mnt_info.node);
    /*
     * _files_hash is process-global state for this device server. Freeing it
     * here races with delayed close notifications that can still arrive while
     * the process is unwinding. Let process exit reclaim the heap instead.
     */
    _files_hash = NULL;
    return 0;
}

#ifdef __cplusplus
}
#endif
