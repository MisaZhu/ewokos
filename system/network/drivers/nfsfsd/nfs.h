#ifndef NFSFS_NFS_H
#define NFSFS_NFS_H

#include <stdint.h>
#include <stddef.h>

/* ONC RPC / 程序号 (RFC 1057 / 1813 / 1813 mount) */
#define PMAP_PROGRAM   100000
#define PMAP_V2        2
#define PMAP_GETPORT   3
#define NFS_PROGRAM    100003
#define NFS_V3         3
#define MOUNT_PROGRAM  100005
#define MOUNT_V3       3
#define MOUNT_MNT      1
#define MOUNT_UMNT     3

#define RPC_CALL       0
#define RPC_REPLY      1
#define RPC_MSG_ACCEPTED 0
#define RPC_SUCCESS    0
#define AUTH_NULL      0
#define AUTH_UNIX      1

/* NFS v3 过程号 (RFC 1813) */
#define NFSPROC3_NULL        0
#define NFSPROC3_GETATTR     1
#define NFSPROC3_SETATTR     2
#define NFSPROC3_LOOKUP      3
#define NFSPROC3_ACCESS      4
#define NFSPROC3_READ        6
#define NFSPROC3_WRITE       7
#define NFSPROC3_CREATE      8
#define NFSPROC3_MKDIR       9
#define NFSPROC3_REMOVE      12
#define NFSPROC3_RMDIR       13
#define NFSPROC3_READDIRPLUS 17

/* NFS v3 状态码 */
#define NFS3_OK             0
#define NFS3ERR_PERM        1
#define NFS3ERR_NOENT       2
#define NFS3ERR_IO          5
#define NFS3ERR_ACCES       13
#define NFS3ERR_EXIST       17
#define NFS3ERR_NOTDIR      20
#define NFS3ERR_ISDIR       21
#define NFS3ERR_INVAL       22
#define NFS3ERR_FBIG        27
#define NFS3ERR_NOSPC       28
#define NFS3ERR_ROFS        30
#define NFS3ERR_NAMETOOLONG 63
#define NFS3ERR_NOTEMPTY    66
#define NFS3ERR_STALE       70
#define NFS3ERR_BADHANDLE   10001
#define NFS3ERR_NOTSUPP     10004

/* NFS v3 文件类型 */
#define NF3REG  1
#define NF3DIR  2
#define NF3BLK  3
#define NF3CHR  4
#define NF3LNK  5
#define NF3SOCK 6
#define NF3FIFO 7

#define NFS_FH3_MAX   64
#define NFS_MAXDATA   (1024 * 1024)   /* 单次 READ/WRITE 上限(TCP 流式;FSINFO rtmax 协商前的保守上限) */
#define NFS_MAXPATH   1024
#define NFS_MAXNAME   255

typedef struct { uint32_t len; uint8_t data[NFS_FH3_MAX]; } nfs_fh3;
typedef struct { uint32_t sec; uint32_t usec; } nfstime3;

typedef struct {
    uint32_t type;
    uint32_t mode;
    uint32_t nlink;
    uint32_t uid;
    uint32_t gid;
    uint64_t size;
    uint64_t used;
    uint32_t rdev1;
    uint32_t rdev2;
    uint64_t fsid;
    uint64_t fileid;
    nfstime3 atime;
    nfstime3 mtime;
    nfstime3 ctime;
} fattr3;

/* ---------------- XDR (RFC 1014, 大端) ---------------- */
typedef struct {
    uint8_t* buf;
    uint32_t len;       /* 已写入/可读字节 */
    uint32_t pos;       /* 读游标 */
    uint32_t cap;
    int      err;
} xdr_t;

void     xdr_init(xdr_t* x, uint8_t* buf, uint32_t cap);
void     xdr_put_u32(xdr_t* x, uint32_t v);
void     xdr_put_u64(xdr_t* x, uint64_t v);
void     xdr_put_opaque(xdr_t* x, const void* data, uint32_t len);
void     xdr_put_string(xdr_t* x, const char* s);
uint32_t xdr_get_u32(xdr_t* x);
uint64_t xdr_get_u64(xdr_t* x);
uint32_t xdr_get_opaque(xdr_t* x, void* out, uint32_t max);
/* 返回 malloc 字符串(调用者 free);失败 NULL */
char*    xdr_get_string(xdr_t* x);
void     xdr_skip(xdr_t* x, uint32_t n);
int      xdr_ok(const xdr_t* x);

/* fattr3 / post_op_attr / fh3 编解码 */
void xdr_put_fh3(xdr_t* x, const nfs_fh3* fh);
int  xdr_get_fh3(xdr_t* x, nfs_fh3* fh);
void xdr_get_fattr3(xdr_t* x, fattr3* a);
/* post_op_attr: bool + 可选 fattr3;返回是否跟随属性 */
int  xdr_get_post_attr(xdr_t* x, fattr3* a);
void xdr_get_nfstime3(xdr_t* x, nfstime3* t);

/* ---------------- ONC RPC over UDP ---------------- */
typedef struct {
    int      sock;
    int      tcp_sock;      /* NFS over TCP 连接(-1 未建) */
    int      use_tcp;       /* NFS 过程走 TCP */
    uint32_t server_ip;     /* 网络序 */
    uint16_t nfs_port;      /* 主机序 */
    uint16_t mount_port;
    uint32_t xid;
    uint32_t uid;           /* AUTH_UNIX */
    uint32_t gid;
    int      timeout_ms;
    int      retries;
} rpc_client_t;

int  rpc_init(rpc_client_t* c, uint32_t server_ip, int timeout_ms, int retries);
void rpc_close(rpc_client_t* c);
/* 建立 NFS over TCP 连接(流式,无 UDP datagram 尺寸限制,大块读高效) */
int  rpc_tcp_connect(rpc_client_t* c, uint16_t port);
/* portmapper GETPORT:返回端口(主机序),失败 0 */
uint16_t rpc_getport(rpc_client_t* c, uint32_t prog, uint32_t vers, uint32_t proto);
/* 发起 RPC 调用:payload 为已编码的过程参数;reply 缓冲接收回复体(去掉 RPC 头)。
   返回回复体长度,<0 失败。prog/vers/proc 指定目标。cred 用 AUTH_UNIX(uid/gid)。*/
int  rpc_call(rpc_client_t* c, uint16_t port, uint32_t prog, uint32_t vers,
              uint32_t proc, const uint8_t* args, uint32_t args_len,
              uint8_t* reply, uint32_t reply_cap);

/* ---------------- NFS v3 过程 ---------------- */
typedef struct {
    rpc_client_t rpc;
    nfs_fh3      root_fh;
    fattr3       root_attr;
    uint32_t     uid;
    uint32_t     gid;
    uint32_t     rtmax;       /* 服务端建议读上限 */
    uint32_t     wtmax;       /* 写上限 */
} nfs_client_t;

/* mount/umount */
int nfs_mount(nfs_client_t* n, const char* export_path);
int nfs_umount(nfs_client_t* n, const char* export_path);

int nfs_getattr(nfs_client_t* n, const nfs_fh3* fh, fattr3* a);
/* FSINFO:取服务端 rtmax/wtmax(读/写 preferred 上限),失败保持默认 */
int nfs_fsinfo(nfs_client_t* n, const nfs_fh3* fh);
int nfs_lookup(nfs_client_t* n, const nfs_fh3* dir, const char* name,
               nfs_fh3* out_fh, fattr3* out_attr);
int nfs_read(nfs_client_t* n, const nfs_fh3* fh, uint64_t offset,
             uint32_t count, void* buf, uint32_t* got, int* eof);
int nfs_write(nfs_client_t* n, const nfs_fh3* fh, uint64_t offset,
              const void* buf, uint32_t count, uint32_t* wrote);
int nfs_readdirplus(nfs_client_t* n, const nfs_fh3* dir, uint64_t cookie,
                    void* ctx, int (*cb)(void* ctx, const char* name,
                                         const nfs_fh3* fh, const fattr3* a,
                                         uint64_t cookie, int eof));
int nfs_create(nfs_client_t* n, const nfs_fh3* dir, const char* name,
               uint32_t mode, nfs_fh3* out_fh, fattr3* out_attr);
int nfs_mkdir(nfs_client_t* n, const nfs_fh3* dir, const char* name,
              uint32_t mode, nfs_fh3* out_fh, fattr3* out_attr);
int nfs_remove(nfs_client_t* n, const nfs_fh3* dir, const char* name);
int nfs_rmdir(nfs_client_t* n, const nfs_fh3* dir, const char* name);

/* NFS 状态 → errno 风格负值 */
int nfs_status_to_err(uint32_t status);

#endif