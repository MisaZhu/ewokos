/*
 * nfsfsd — EwokOS 用户态 NFS v3 客户端文件系统守护进程。
 * 仿 ramfsd/sdfsd:实现 vdevice_t 回调,经 libsocket UDP 跑 ONC RPC/XDR 与
 * NFS 服务器通信,device_run 挂载到命名空间(如 /mnt/nfs)。运行时挂载,无需改内核/vfsd。
 * 用法: nfsfsd <挂载点> <server:export>   例: nfsfsd /mnt/nfs 192.168.8.1:/srv/nfs
 * 依赖 netd 已启动(/dev/net0,DHCP 获址)。
 */
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <ewoksys/klog.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "nfs.h"

typedef struct {
    nfs_fh3 fh;
    fattr3  attr;
    /* readahead window: GGUF-style metadata is thousands of tiny sequential
       reads; without this each becomes a full NFS RPC round-trip. */
    uint8_t*  rc_buf;
    uint64_t  rc_off;
    uint32_t  rc_len;
} nfs_node_t;

static nfs_client_t g_nfs;

static uint32_t nf3_type_to_fs(uint32_t t) {
    switch(t) {
    case NF3DIR:  return FS_TYPE_DIR;
    case NF3REG:  return FS_TYPE_FILE;
    case NF3LNK:  return FS_TYPE_LINK;
    case NF3BLK:  return FS_TYPE_BLOCK;
    case NF3CHR:  return FS_TYPE_CHAR;
    case NF3FIFO: return FS_TYPE_PIPE;
    default:      return FS_TYPE_FILE;
    }
}

static void fattr3_to_stat(const fattr3* a, node_stat_t* s) {
    memset(s, 0, sizeof(*s));
    s->mode = (uint16_t)(a->mode & 07777);   /* 权限位;S_IF* 由 libc 从 fsinfo.type 合成 */
    s->uid = (uint16_t)a->uid;
    s->gid = (uint16_t)a->gid;
    s->size = (off_t)a->size;
    s->atime = a->atime.sec;
    s->ctime = a->ctime.sec;
    s->mtime = a->mtime.sec;
    s->links_count = (uint16_t)a->nlink;
}

static nfs_node_t* node_new(const nfs_fh3* fh, const fattr3* a) {
    nfs_node_t* n = (nfs_node_t*)malloc(sizeof(nfs_node_t));
    if(n) {
        memset(n, 0, sizeof(*n));
        if(fh) n->fh = *fh;
        if(a) n->attr = *a;
    }
    return n;
}

static nfs_node_t* get_node(fsinfo_t* info) {
    return (nfs_node_t*)(ewokos_addr_t)info->data;
}

/* ---------------- mount 回调:置根节点 ---------------- */
static int nfsd_mount(vdevice_t* dev, fsinfo_t* info, void* p) {
    (void)dev;
    nfs_client_t* n = (nfs_client_t*)p;
    nfs_node_t* root = node_new(&n->root_fh, &n->root_attr);
    if(!root)
        return -1;
    info->data = (ewokos_addr_t)root;
    info->type = FS_TYPE_DIR;
    fattr3_to_stat(&n->root_attr, &info->stat);
    return 0;
}

/* ---------------- kids: READDIRPLUS → fsinfo 数组 ---------------- */
typedef struct {
    fsinfo_t* arr;
    uint32_t num;
    uint32_t cap;
} kids_ctx_t;

static int kids_cb(void* ctx, const char* name, const nfs_fh3* fh,
                   const fattr3* a, uint64_t cookie, int eof) {
    (void)cookie; (void)eof;
    kids_ctx_t* k = (kids_ctx_t*)ctx;
    if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        return 0;
    uint32_t type = nf3_type_to_fs(a->type);
    if(type != FS_TYPE_DIR && type != FS_TYPE_FILE && type != FS_TYPE_LINK)
        return 0;
    if(k->num >= k->cap) {
        k->cap = k->cap ? k->cap * 2 : 16;
        k->arr = (fsinfo_t*)realloc(k->arr, sizeof(fsinfo_t) * k->cap);
        if(!k->arr) return -1;
    }
    fsinfo_t* f = &k->arr[k->num];
    memset(f, 0, sizeof(*f));
    strncpy(f->name, name, FS_NODE_NAME_MAX - 1);
    f->type = type;
    fattr3_to_stat(a, &f->stat);
    if(fh) {
        nfs_node_t* nn = node_new(fh, a);
        if(nn) f->data = (ewokos_addr_t)nn;
    }
    k->num++;
    return 0;
}

static fsinfo_t* nfsd_kids(vdevice_t* dev, fsinfo_t* info_dir, uint32_t* num, void* p) {
    (void)dev;
    *num = 0;
    nfs_client_t* n = (nfs_client_t*)p;
    if(!FS_IS_TYPE(info_dir->type, FS_TYPE_DIR))
        return NULL;
    nfs_node_t* dn = get_node(info_dir);
    const nfs_fh3* dfh = dn ? &dn->fh : &n->root_fh;
    kids_ctx_t k;
    memset(&k, 0, sizeof(k));
    if(nfs_readdirplus(n, dfh, 0, &k, kids_cb) != 0) {
        free(k.arr);
        return NULL;
    }
    *num = k.num;
    return k.arr;
}

/* ---------------- get: 全路径 LOOKUP(从根分解组件) ---------------- */
static int nfsd_get(vdevice_t* dev, int from_pid, const char* fname,
                   fsinfo_t* info, void* p) {
    (void)dev; (void)from_pid;
    nfs_client_t* n = (nfs_client_t*)p;
    memset(info, 0, sizeof(*info));
    nfs_fh3 cur = n->root_fh;
    fattr3 cur_attr = n->root_attr;
    char path[NFS_MAXPATH];
    strncpy(path, fname, NFS_MAXPATH - 1);
    path[NFS_MAXPATH - 1] = 0;
    char* save = NULL;
    char* comp = strtok_r(path, "/", &save);
    if(comp == NULL) {                  /* 根 */
        nfs_node_t* root = node_new(&n->root_fh, &n->root_attr);
        if(!root) return -1;
        strcpy(info->name, "/");
        info->type = FS_TYPE_DIR;
        info->data = (ewokos_addr_t)root;
        fattr3_to_stat(&n->root_attr, &info->stat);
        return 0;
    }
    while(comp) {
        nfs_fh3 child_fh;
        fattr3 child_attr;
        if(nfs_lookup(n, &cur, comp, &child_fh, &child_attr) != 0)
            return -1;
        cur = child_fh;
        cur_attr = child_attr;
        char* next = strtok_r(NULL, "/", &save);
        if(next == NULL) {
            nfs_node_t* nn = node_new(&cur, &cur_attr);
            if(!nn) return -1;
            strncpy(info->name, comp, FS_NODE_NAME_MAX - 1);
            info->type = nf3_type_to_fs(cur_attr.type);
            info->data = (ewokos_addr_t)nn;
            fattr3_to_stat(&cur_attr, &info->stat);
            return 0;
        }
        comp = next;
    }
    return -1;
}

/* ---------------- stat ---------------- */
static int nfsd_stat(vdevice_t* dev, int from_pid, fsinfo_t* info,
                    node_stat_t* stat, void* p) {
    (void)dev; (void)from_pid;
    nfs_client_t* n = (nfs_client_t*)p;
    nfs_node_t* nn = get_node(info);
    const nfs_fh3* fh = nn ? &nn->fh : &n->root_fh;
    fattr3 a;
    if(nfs_getattr(n, fh, &a) == 0) {
        fattr3_to_stat(&a, stat);
        if(nn) nn->attr = a;
        return 0;
    }
    *stat = info->stat;                 /* 回落缓存 */
    return 0;
}

/* ---------------- read/write ---------------- */
static int nfsd_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
                    void* buf, int size, off_t offset, void* p) {
    (void)dev; (void)fd; (void)from_pid;
    nfs_client_t* n = (nfs_client_t*)p;
    nfs_node_t* nn = get_node(info);
    if(!nn || size <= 0)
        return 0;
    if(offset < 0) offset = 0;
    uint64_t uoff = (uint64_t)offset;
    uint32_t got = 0;
    int eof = 0;
    uint32_t want = (uint32_t)size;
    uint32_t rcap = n->rtmax ? n->rtmax : NFS_MAXDATA;
    if(want > rcap) want = rcap;

    /* serve from the readahead window */
    if(nn->rc_buf && uoff >= nn->rc_off && uoff < nn->rc_off + nn->rc_len) {
        uint32_t avail = (uint32_t)(nn->rc_off + nn->rc_len - uoff);
        uint32_t take = want < avail ? want : avail;
        memcpy(buf, nn->rc_buf + (uoff - nn->rc_off), take);
        return (int)take;
    }
    /* bulk reads bypass the cache (weight tensors etc.) */
    if(want >= 256 * 1024) {
        if(nfs_read(n, &nn->fh, uoff, want, buf, &got, &eof) != 0)
            return -1;
        return (int)got;
    }
    /* small read: fetch a full window ahead */
    if(!nn->rc_buf)
        nn->rc_buf = (uint8_t*)malloc(rcap);
    if(nn->rc_buf) {
        if(nfs_read(n, &nn->fh, uoff, rcap, nn->rc_buf, &got, &eof) == 0 && got > 0) {
            nn->rc_off = uoff;
            nn->rc_len = got;
            uint32_t take = want < got ? want : got;
            memcpy(buf, nn->rc_buf, take);
            return (int)take;
        }
    }
    if(nfs_read(n, &nn->fh, uoff, want, buf, &got, &eof) != 0)
        return -1;
    return (int)got;
}

static int nfsd_write(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
                     const void* buf, int size, off_t offset, void* p) {
    (void)dev; (void)fd; (void)from_pid;
    nfs_client_t* n = (nfs_client_t*)p;
    nfs_node_t* nn = get_node(info);
    if(!nn || size <= 0)
        return size;
    if(offset < 0) offset = 0;
    nn->rc_len = 0;                     /* invalidate readahead on write */
    uint32_t wrote = 0;
    uint32_t want = (uint32_t)size;
    uint32_t wcap = n->wtmax ? n->wtmax : NFS_MAXDATA;
    if(want > wcap) want = wcap;
    if(nfs_write(n, &nn->fh, (uint64_t)offset, buf, want, &wrote) != 0)
        return -1;
    return (int)wrote;
}

/* ---------------- create/unlink ---------------- */
static int nfsd_create(vdevice_t* dev, int from_pid, fsinfo_t* info_to,
                      fsinfo_t* info, void* p) {
    (void)dev; (void)from_pid;
    nfs_client_t* n = (nfs_client_t*)p;
    nfs_node_t* parent = get_node(info_to);
    const nfs_fh3* dir = parent ? &parent->fh : &n->root_fh;
    uint32_t mode = info->stat.mode ? (info->stat.mode & 07777) : 0644;
    nfs_fh3 fh;
    fattr3 a;
    int ret;
    if(FS_IS_TYPE(info->type, FS_TYPE_DIR))
        ret = nfs_mkdir(n, dir, info->name, mode | 0755, &fh, &a);
    else
        ret = nfs_create(n, dir, info->name, mode, &fh, &a);
    if(ret != 0)
        return -1;
    nfs_node_t* nn = node_new(&fh, &a);
    if(!nn) return -1;
    info->data = (ewokos_addr_t)nn;
    fattr3_to_stat(&a, &info->stat);
    return 0;
}

static int nfsd_unlink(vdevice_t* dev, fsinfo_t* info, const char* fname, void* p) {
    (void)dev;
    nfs_client_t* n = (nfs_client_t*)p;
    nfs_node_t* parent = get_node(info);
    const nfs_fh3* dir = parent ? &parent->fh : &n->root_fh;
    /* info 为父目录;fname 为待删名。需先 lookup 判类型 */
    nfs_fh3 fh;
    fattr3 a;
    int ret;
    if(nfs_lookup(n, dir, fname, &fh, &a) == 0 && a.type == NF3DIR)
        ret = nfs_rmdir(n, dir, fname);
    else
        ret = nfs_remove(n, dir, fname);
    if(ret != 0)
        return -1;
    return vfs_del_node(info->node);
}

static int nfsd_open(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
                    int oflag, void* p) {
    (void)dev; (void)fd; (void)from_pid; (void)info; (void)oflag; (void)p;
    return 0;                           /* NFS 无状态;O_TRUNC 经 SETATTR 后续支持 */
}

static int nfsd_close(vdevice_t* dev, int fd, int from_pid, ewokos_addr_t node,
                     fsinfo_t* info, void* p) {
    (void)dev; (void)fd; (void)from_pid; (void)node; (void)info; (void)p;
    return 0;
}

int main(int argc, char** argv) {
    if(argc < 3) {
        slog("usage: nfsfsd <mnt_point> <server:export>\n");
        return 1;
    }
    const char* mnt_point = argv[1];
    char spec[NFS_MAXPATH];
    strncpy(spec, argv[2], NFS_MAXPATH - 1);
    spec[NFS_MAXPATH - 1] = 0;
    char* colon = strchr(spec, ':');
    if(!colon) {
        slog("nfsfsd: spec must be server:export\n");
        return 1;
    }
    *colon = 0;
    const char* server = spec;
    const char* export_path = colon + 1;

    uint32_t ip = inet_addr(server);
    if(ip == 0xFFFFFFFF || ip == 0) {
        struct hostent* he = gethostbyname(server);
        if(!he || !he->h_addr_list[0]) {
            slog("nfsfsd: cannot resolve server %s\n", server);
            return 1;
        }
        memcpy(&ip, he->h_addr_list[0], 4);
    }

    memset(&g_nfs, 0, sizeof(g_nfs));
    g_nfs.uid = 0;
    g_nfs.gid = 0;
    if(rpc_init(&g_nfs.rpc, ip, 3000, 4) != 0) {
        slog("nfsfsd: socket init failed (netd up?)\n");
        return 1;
    }
    g_nfs.rpc.uid = 0;
    g_nfs.rpc.gid = 0;
    if(nfs_mount(&g_nfs, export_path) != 0) {
        slog("nfsfsd: mount %s:%s failed\n", server, export_path);
        rpc_close(&g_nfs.rpc);
        return 1;
    }
    slog("nfsfsd: mounted %s:%s at %s\n", server, export_path, mnt_point);

    vdevice_t dev;
    memset(&dev, 0, sizeof(dev));
    strcpy(dev.desc, "nfs");
    dev.extra_data = &g_nfs;
    dev.mount = nfsd_mount;
    dev.get = nfsd_get;
    dev.kids = nfsd_kids;
    dev.stat = nfsd_stat;
    dev.read = nfsd_read;
    dev.write = nfsd_write;
    dev.create = nfsd_create;
    dev.unlink = nfsd_unlink;
    dev.open = nfsd_open;
    dev.close = nfsd_close;

    device_run(&dev, mnt_point, FS_TYPE_DIR, 0755, true);
    nfs_umount(&g_nfs, export_path);
    rpc_close(&g_nfs.rpc);
    return 0;
}