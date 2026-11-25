#include "nfs.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>

#define NFS_COOKIEVERF_SIZE 8

int nfs_status_to_err(uint32_t s) {
    switch(s) {
    case NFS3_OK: return 0;
    case NFS3ERR_PERM: return -EPERM;
    case NFS3ERR_NOENT: return -ENOENT;
    case NFS3ERR_IO: return -EIO;
    case NFS3ERR_ACCES: return -EACCES;
    case NFS3ERR_EXIST: return -EEXIST;
    case NFS3ERR_NOTDIR: return -ENOTDIR;
    case NFS3ERR_ISDIR: return -EISDIR;
    case NFS3ERR_INVAL: return -EINVAL;
    case NFS3ERR_FBIG: return -EFBIG;
    case NFS3ERR_NOSPC: return -ENOSPC;
    case NFS3ERR_ROFS: return -EROFS;
    case NFS3ERR_NAMETOOLONG: return -ENAMETOOLONG;
    case NFS3ERR_NOTEMPTY: return -ENOTEMPTY;
    case NFS3ERR_STALE: return -ESTALE;
    default: return -EIO;
    }
}

/* 跳过 wcc_data: pre_op_attr(bool+wcc_attr) + post_op_attr */
static void skip_wcc(xdr_t* r) {
    uint32_t pre = xdr_get_u32(r);
    if(pre) {
        xdr_get_u64(r);             /* size */
        xdr_skip(r, 16);            /* mtime + ctime (各 sec+usec) */
    }
    fattr3 dummy;
    xdr_get_post_attr(r, &dummy);
}

static int nfs_rpc(nfs_client_t* n, uint32_t proc, const uint8_t* args,
                   uint32_t args_len, uint8_t* reply, uint32_t reply_cap) {
    return rpc_call(&n->rpc, n->rpc.nfs_port, NFS_PROGRAM, NFS_V3, proc,
                    args, args_len, reply, reply_cap);
}

int nfs_mount(nfs_client_t* n, const char* export_path) {
    /* 端口:portmapper 查询;NFS 回落 2049 */
    uint16_t mp = rpc_getport(&n->rpc, MOUNT_PROGRAM, MOUNT_V3, IPPROTO_UDP);
    if(mp == 0)
        return -EIO;
    n->rpc.mount_port = mp;
    uint16_t np = rpc_getport(&n->rpc, NFS_PROGRAM, NFS_V3, IPPROTO_UDP);
    n->rpc.nfs_port = np ? np : 2049;
    /* NFS 过程优先 TCP(流式,大块读高效);失败则回退 UDP */
    rpc_tcp_connect(&n->rpc, n->rpc.nfs_port);

    uint8_t args[NFS_MAXPATH + 8];
    xdr_t a;
    xdr_init(&a, args, sizeof(args));
    xdr_put_string(&a, export_path);
    uint8_t reply[NFS_FH3_MAX + 128];
    int rn = rpc_call(&n->rpc, mp, MOUNT_PROGRAM, MOUNT_V3, MOUNT_MNT,
                      args, a.len, reply, sizeof(reply));
    if(rn < 4)
        return -EIO;
    xdr_t r;
    xdr_init(&r, reply, (uint32_t)rn);
    r.len = (uint32_t)rn;
    uint32_t status = xdr_get_u32(&r);
    if(status != NFS3_OK)
        return nfs_status_to_err(status);
    if(xdr_get_fh3(&r, &n->root_fh) != 0)
        return -EIO;
    /* auth_flavors 数组(忽略) */
    if(nfs_getattr(n, &n->root_fh, &n->root_attr) != 0)
        return -EIO;
    nfs_fsinfo(n, &n->root_fh);         /* 协商读/写上限,失败保持默认 */
    return 0;
}

int nfs_umount(nfs_client_t* n, const char* export_path) {
    if(n->rpc.mount_port == 0)
        return 0;
    uint8_t args[NFS_MAXPATH + 8];
    xdr_t a;
    xdr_init(&a, args, sizeof(args));
    xdr_put_string(&a, export_path);
    uint8_t reply[8];
    rpc_call(&n->rpc, n->rpc.mount_port, MOUNT_PROGRAM, MOUNT_V3, MOUNT_UMNT,
             args, a.len, reply, sizeof(reply));
    return 0;
}

int nfs_getattr(nfs_client_t* n, const nfs_fh3* fh, fattr3* a) {
    uint8_t args[NFS_FH3_MAX + 8];
    xdr_t x;
    xdr_init(&x, args, sizeof(args));
    xdr_put_fh3(&x, fh);
    uint8_t reply[256];
    int rn = nfs_rpc(n, NFSPROC3_GETATTR, args, x.len, reply, sizeof(reply));
    if(rn < 4)
        return -EIO;
    xdr_t r;
    xdr_init(&r, reply, (uint32_t)rn);
    r.len = (uint32_t)rn;
    uint32_t status = xdr_get_u32(&r);
    if(status != NFS3_OK)
        return nfs_status_to_err(status);
    xdr_get_fattr3(&r, a);
    return xdr_ok(&r) ? 0 : -EIO;
}

int nfs_fsinfo(nfs_client_t* n, const nfs_fh3* fh) {
    uint8_t args[NFS_FH3_MAX + 8];
    xdr_t x;
    xdr_init(&x, args, sizeof(args));
    xdr_put_fh3(&x, fh);
    uint8_t reply[256];
    int rn = nfs_rpc(n, 19 /* FSINFO */, args, x.len, reply, sizeof(reply));
    if(rn < 4)
        return -EIO;
    xdr_t r;
    xdr_init(&r, reply, (uint32_t)rn);
    r.len = (uint32_t)rn;
    uint32_t status = xdr_get_u32(&r);
    if(status != NFS3_OK)
        return nfs_status_to_err(status);
    fattr3 fa;
    xdr_get_post_attr(&r, &fa);
    uint32_t rtmax = xdr_get_u32(&r);
    uint32_t rtpref = xdr_get_u32(&r);
    xdr_get_u32(&r);                    /* rtmult */
    uint32_t wtmax = xdr_get_u32(&r);
    uint32_t wtpref = xdr_get_u32(&r);
    if(!xdr_ok(&r))
        return -EIO;
    if(rtpref > 0 && rtpref <= rtmax)
        n->rtmax = rtpref < NFS_MAXDATA ? rtpref : NFS_MAXDATA;
    else if(rtmax > 0)
        n->rtmax = rtmax < NFS_MAXDATA ? rtmax : NFS_MAXDATA;
    if(wtpref > 0 && wtpref <= wtmax)
        n->wtmax = wtpref < NFS_MAXDATA ? wtpref : NFS_MAXDATA;
    else if(wtmax > 0)
        n->wtmax = wtmax < NFS_MAXDATA ? wtmax : NFS_MAXDATA;
    return 0;
}

int nfs_lookup(nfs_client_t* n, const nfs_fh3* dir, const char* name,
               nfs_fh3* out_fh, fattr3* out_attr) {
    uint8_t args[NFS_FH3_MAX + NFS_MAXNAME + 16];
    xdr_t x;
    xdr_init(&x, args, sizeof(args));
    xdr_put_fh3(&x, dir);
    xdr_put_string(&x, name);
    uint8_t reply[NFS_FH3_MAX + 512];
    int rn = nfs_rpc(n, NFSPROC3_LOOKUP, args, x.len, reply, sizeof(reply));
    if(rn < 4)
        return -EIO;
    xdr_t r;
    xdr_init(&r, reply, (uint32_t)rn);
    r.len = (uint32_t)rn;
    uint32_t status = xdr_get_u32(&r);
    if(status != NFS3_OK)
        return nfs_status_to_err(status);
    if(xdr_get_fh3(&r, out_fh) != 0)
        return -EIO;
    xdr_get_post_attr(&r, out_attr);     /* obj attributes */
    fattr3 dir_attr;
    xdr_get_post_attr(&r, &dir_attr);    /* dir attributes */
    return xdr_ok(&r) ? 0 : -EIO;
}

int nfs_read(nfs_client_t* n, const nfs_fh3* fh, uint64_t offset,
             uint32_t count, void* buf, uint32_t* got, int* eof) {
    uint8_t args[NFS_FH3_MAX + 32];
    xdr_t x;
    xdr_init(&x, args, sizeof(args));
    xdr_put_fh3(&x, fh);
    xdr_put_u64(&x, offset);
    xdr_put_u32(&x, count);
    uint32_t reply_cap = count + 256;
    uint8_t* reply = (uint8_t*)malloc(reply_cap);
    if(!reply)
        return -ENOMEM;
    int rn = nfs_rpc(n, NFSPROC3_READ, args, x.len, reply, reply_cap);
    int ret = -EIO;
    if(rn >= 4) {
        xdr_t r;
        xdr_init(&r, reply, (uint32_t)rn);
        r.len = (uint32_t)rn;
        uint32_t status = xdr_get_u32(&r);
        if(status != NFS3_OK) {
            ret = nfs_status_to_err(status);
        } else {
            fattr3 fa;
            xdr_get_post_attr(&r, &fa);
            uint32_t nread = xdr_get_u32(&r);
            uint32_t e = xdr_get_u32(&r);
            uint32_t dlen = xdr_get_opaque(&r, buf, count);
            if(xdr_ok(&r)) {
                *got = dlen < nread ? dlen : nread;
                *eof = e ? 1 : 0;
                ret = 0;
            }
        }
    }
    free(reply);
    return ret;
}

int nfs_write(nfs_client_t* n, const nfs_fh3* fh, uint64_t offset,
              const void* buf, uint32_t count, uint32_t* wrote) {
    uint32_t args_cap = NFS_FH3_MAX + count + 64;
    uint8_t* args = (uint8_t*)malloc(args_cap);
    if(!args)
        return -ENOMEM;
    xdr_t x;
    xdr_init(&x, args, args_cap);
    xdr_put_fh3(&x, fh);
    xdr_put_u64(&x, offset);
    xdr_put_u32(&x, count);
    xdr_put_u32(&x, 2);                 /* FILE_SYNC */
    xdr_put_opaque(&x, buf, count);
    uint8_t reply[256];
    int rn = nfs_rpc(n, NFSPROC3_WRITE, args, x.len, reply, sizeof(reply));
    free(args);
    if(rn < 4)
        return -EIO;
    xdr_t r;
    xdr_init(&r, reply, (uint32_t)rn);
    r.len = (uint32_t)rn;
    uint32_t status = xdr_get_u32(&r);
    if(status != NFS3_OK)
        return nfs_status_to_err(status);
    skip_wcc(&r);
    uint32_t nwrite = xdr_get_u32(&r);
    xdr_get_u32(&r);                    /* committed */
    xdr_skip(&r, 8);                    /* verf */
    if(!xdr_ok(&r))
        return -EIO;
    *wrote = nwrite;
    return 0;
}

int nfs_readdirplus(nfs_client_t* n, const nfs_fh3* dir, uint64_t cookie,
                    void* ctx, int (*cb)(void* ctx, const char* name,
                                         const nfs_fh3* fh, const fattr3* a,
                                         uint64_t cookie, int eof)) {
    uint8_t cookieverf[NFS_COOKIEVERF_SIZE];
    memset(cookieverf, 0, sizeof(cookieverf));
    uint64_t ck = cookie;
    uint32_t maxcount = NFS_MAXDATA;
    for(;;) {
        uint8_t args[NFS_FH3_MAX + 64];
        xdr_t x;
        xdr_init(&x, args, sizeof(args));
        xdr_put_fh3(&x, dir);
        xdr_put_u64(&x, ck);
        /* cookieverf3 是定长 8 字节 opaque(无长度前缀) */
        memcpy(args + x.len, cookieverf, NFS_COOKIEVERF_SIZE);
        x.len += NFS_COOKIEVERF_SIZE;
        xdr_put_u32(&x, 4096);               /* dircount */
        xdr_put_u32(&x, maxcount);           /* maxcount */

        uint32_t reply_cap = maxcount + 1024;
        uint8_t* reply = (uint8_t*)malloc(reply_cap);
        if(!reply)
            return -ENOMEM;
        int rn = nfs_rpc(n, NFSPROC3_READDIRPLUS, args, x.len, reply, reply_cap);
        int ret = -EIO;
        int eof = 0;
        uint64_t last_ck = ck;
        int entries = 0;
        if(rn >= 4) {
            xdr_t r;
            xdr_init(&r, reply, (uint32_t)rn);
            r.len = (uint32_t)rn;
            uint32_t status = xdr_get_u32(&r);
            if(status != NFS3_OK) {
                ret = nfs_status_to_err(status);
            } else {
                fattr3 da;
                xdr_get_post_attr(&r, &da);
                xdr_skip(&r, NFS_COOKIEVERF_SIZE);   /* cookieverf */
                while(xdr_ok(&r) && xdr_get_u32(&r) == 1) {
                    xdr_get_u64(&r);                 /* fileid */
                    char* name = xdr_get_string(&r);
                    uint64_t eck = xdr_get_u64(&r);  /* cookie */
                    fattr3 ea;
                    xdr_get_post_attr(&r, &ea);
                    nfs_fh3 efh;
                    uint32_t has_fh = xdr_get_u32(&r);
                    if(has_fh)
                        xdr_get_fh3(&r, &efh);
                    else
                        memset(&efh, 0, sizeof(efh));
                    if(xdr_ok(&r) && name) {
                        last_ck = eck;
                        entries++;
                        int cr = cb(ctx, name, has_fh ? &efh : NULL, &ea, eck, 0);
                        free(name);
                        if(cr != 0) { eof = 1; break; }
                    } else {
                        free(name);
                        break;
                    }
                }
                if(xdr_ok(&r))
                    eof = xdr_get_u32(&r) ? 1 : eof;
                ret = 0;
            }
        }
        free(reply);
        if(ret != 0)
            return ret;
        if(eof || entries == 0 || last_ck == ck)
            return 0;
        ck = last_ck;
    }
}

/* CREATE/MKDIR 共用:返回 obj fh + attr */
static int nfs_make(nfs_client_t* n, uint32_t proc, const nfs_fh3* dir,
                    const char* name, uint32_t mode, nfs_fh3* out_fh,
                    fattr3* out_attr, int is_dir) {
    uint8_t args[NFS_FH3_MAX + NFS_MAXNAME + 128];
    xdr_t x;
    xdr_init(&x, args, sizeof(args));
    xdr_put_fh3(&x, dir);
    xdr_put_string(&x, name);
    if(is_dir) {
        /* sattr3: mode/uid/gid/size/atime/mtime 各 bool+value */
        xdr_put_u32(&x, 1); xdr_put_u32(&x, mode);   /* mode */
        xdr_put_u32(&x, 0);                          /* uid */
        xdr_put_u32(&x, 0);                          /* gid */
        xdr_put_u32(&x, 0);                          /* size */
        xdr_put_u32(&x, 0);                          /* atime */
        xdr_put_u32(&x, 0);                          /* mtime */
    } else {
        xdr_put_u32(&x, 0);                          /* create mode UNCHECKED */
        xdr_put_u32(&x, 1); xdr_put_u32(&x, mode);   /* sattr3.mode */
        xdr_put_u32(&x, 0);                          /* uid */
        xdr_put_u32(&x, 0);                          /* gid */
        xdr_put_u32(&x, 0);                          /* size */
        xdr_put_u32(&x, 0);                          /* atime */
        xdr_put_u32(&x, 0);                          /* mtime */
    }
    uint8_t reply[NFS_FH3_MAX + 512];
    int rn = nfs_rpc(n, proc, args, x.len, reply, sizeof(reply));
    if(rn < 4)
        return -EIO;
    xdr_t r;
    xdr_init(&r, reply, (uint32_t)rn);
    r.len = (uint32_t)rn;
    uint32_t status = xdr_get_u32(&r);
    if(status != NFS3_OK)
        return nfs_status_to_err(status);
    uint32_t has_fh = xdr_get_u32(&r);               /* post_op_fh3 */
    if(has_fh && out_fh)
        xdr_get_fh3(&r, out_fh);
    else if(out_fh)
        memset(out_fh, 0, sizeof(*out_fh));
    if(out_attr)
        xdr_get_post_attr(&r, out_attr);
    skip_wcc(&r);
    return xdr_ok(&r) ? 0 : -EIO;
}

int nfs_create(nfs_client_t* n, const nfs_fh3* dir, const char* name,
               uint32_t mode, nfs_fh3* out_fh, fattr3* out_attr) {
    return nfs_make(n, NFSPROC3_CREATE, dir, name, mode, out_fh, out_attr, 0);
}

int nfs_mkdir(nfs_client_t* n, const nfs_fh3* dir, const char* name,
              uint32_t mode, nfs_fh3* out_fh, fattr3* out_attr) {
    return nfs_make(n, NFSPROC3_MKDIR, dir, name, mode, out_fh, out_attr, 1);
}

static int nfs_del(nfs_client_t* n, uint32_t proc, const nfs_fh3* dir,
                   const char* name) {
    uint8_t args[NFS_FH3_MAX + NFS_MAXNAME + 16];
    xdr_t x;
    xdr_init(&x, args, sizeof(args));
    xdr_put_fh3(&x, dir);
    xdr_put_string(&x, name);
    uint8_t reply[256];
    int rn = nfs_rpc(n, proc, args, x.len, reply, sizeof(reply));
    if(rn < 4)
        return -EIO;
    xdr_t r;
    xdr_init(&r, reply, (uint32_t)rn);
    r.len = (uint32_t)rn;
    uint32_t status = xdr_get_u32(&r);
    if(status != NFS3_OK)
        return nfs_status_to_err(status);
    skip_wcc(&r);
    return 0;
}

int nfs_remove(nfs_client_t* n, const nfs_fh3* dir, const char* name) {
    return nfs_del(n, NFSPROC3_REMOVE, dir, name);
}

int nfs_rmdir(nfs_client_t* n, const nfs_fh3* dir, const char* name) {
    return nfs_del(n, NFSPROC3_RMDIR, dir, name);
}