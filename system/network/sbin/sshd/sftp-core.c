#include "sftp-core.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <ewoksys/vfs.h>

#define SFTP_VERSION 3

#define SSH_FXP_INIT            1
#define SSH_FXP_VERSION         2
#define SSH_FXP_OPEN            3
#define SSH_FXP_CLOSE           4
#define SSH_FXP_READ            5
#define SSH_FXP_WRITE           6
#define SSH_FXP_LSTAT           7
#define SSH_FXP_FSTAT           8
#define SSH_FXP_SETSTAT         9
#define SSH_FXP_FSETSTAT        10
#define SSH_FXP_OPENDIR         11
#define SSH_FXP_READDIR         12
#define SSH_FXP_REMOVE          13
#define SSH_FXP_MKDIR           14
#define SSH_FXP_RMDIR           15
#define SSH_FXP_REALPATH        16
#define SSH_FXP_STAT            17
#define SSH_FXP_RENAME          18
#define SSH_FXP_READLINK        19
#define SSH_FXP_SYMLINK         20
#define SSH_FXP_STATUS          101
#define SSH_FXP_HANDLE          102
#define SSH_FXP_DATA            103
#define SSH_FXP_NAME            104
#define SSH_FXP_ATTRS           105
#define SSH_FXP_EXTENDED        200
#define SSH_FXP_EXTENDED_REPLY  201

#define SSH_FX_OK               0
#define SSH_FX_EOF              1
#define SSH_FX_NO_SUCH_FILE     2
#define SSH_FX_PERMISSION_DENIED 3
#define SSH_FX_FAILURE          4
#define SSH_FX_BAD_MESSAGE      5
#define SSH_FX_OP_UNSUPPORTED   8

#define SSH_FILEXFER_ATTR_SIZE        0x00000001
#define SSH_FILEXFER_ATTR_UIDGID      0x00000002
#define SSH_FILEXFER_ATTR_PERMISSIONS 0x00000004
#define SSH_FILEXFER_ATTR_ACMODTIME   0x00000008

#define SSH_FXF_READ    0x00000001
#define SSH_FXF_WRITE   0x00000002
#define SSH_FXF_APPEND  0x00000004
#define SSH_FXF_CREAT   0x00000008
#define SSH_FXF_TRUNC   0x00000010
#define SSH_FXF_EXCL    0x00000020

#define MAX_HANDLES 64
#define HANDLE_FILE 1
#define HANDLE_DIR  2
#define WRITEBACK_BUFFER_SIZE (64 * 1024)
#define MAX_PKT_SIZE (256 * 1024)
#define MAX_READ_REPLY_DATA (16 * 1024)
#define SFTP_EXT_LIMITS "limits@openssh.com"
#define SFTP_EXT_LIMITS_VER "1"
#define SFTP_LIMITS_MAX_PACKET ((uint64_t)MAX_PKT_SIZE)
#define SFTP_LIMITS_MAX_READ   ((uint64_t)MAX_READ_REPLY_DATA)
#define SFTP_LIMITS_MAX_WRITE  ((uint64_t)(MAX_PKT_SIZE - 1024))
#define SFTP_LIMITS_MAX_OPEN   ((uint64_t)MAX_HANDLES)

typedef struct {
    int type;
    int fd;
    DIR *dirp;
    char path[512];
    uint8_t *wb_buf;
    size_t wb_cap;
    size_t wb_len;
    uint64_t wb_off;
} handle_t;

struct sftp_server {
    handle_t handles[MAX_HANDLES];
    uint8_t *rx_buf;
    size_t rx_len;
    size_t rx_cap;
    uint8_t out_buf[MAX_PKT_SIZE];
    size_t out_off;
    sftp_server_io_t io;
};

static void put_u32(uint8_t *buf, uint32_t val) {
    buf[0] = (val >> 24) & 0xff;
    buf[1] = (val >> 16) & 0xff;
    buf[2] = (val >> 8) & 0xff;
    buf[3] = val & 0xff;
}

static uint32_t get_u32(const uint8_t *buf) {
    return ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] << 8) | (uint32_t)buf[3];
}

static void put_u64(uint8_t *buf, uint64_t val) {
    put_u32(buf, (uint32_t)(val >> 32));
    put_u32(buf + 4, (uint32_t)(val & 0xffffffffu));
}

static uint64_t get_u64(const uint8_t *buf) {
    return ((uint64_t)get_u32(buf) << 32) | (uint64_t)get_u32(buf + 4);
}

static const char *get_string(const uint8_t *buf, size_t buf_len, size_t *off,
        uint32_t *slen) {
    uint32_t len;

    if (*off + 4 > buf_len)
        return NULL;
    len = get_u32(buf + *off);
    *off += 4;
    if (*off + len > buf_len)
        return NULL;
    if (slen)
        *slen = len;
    len = get_u32(buf + *off - 4);
    {
        const char *s = (const char *)(buf + *off);
        *off += len;
        return s;
    }
}

static int get_uint32(const uint8_t *buf, size_t buf_len, size_t *off,
        uint32_t *val) {
    if (*off + 4 > buf_len)
        return -1;
    *val = get_u32(buf + *off);
    *off += 4;
    return 0;
}

static int get_uint64(const uint8_t *buf, size_t buf_len, size_t *off,
        uint64_t *val) {
    if (*off + 8 > buf_len)
        return -1;
    *val = get_u64(buf + *off);
    *off += 8;
    return 0;
}

static void out_begin(sftp_server_t *srv, uint8_t type) {
    srv->out_off = 4;
    srv->out_buf[srv->out_off++] = type;
}

static void out_u32(sftp_server_t *srv, uint32_t val) {
    put_u32(srv->out_buf + srv->out_off, val);
    srv->out_off += 4;
}

static void out_u64(sftp_server_t *srv, uint64_t val) {
    put_u64(srv->out_buf + srv->out_off, val);
    srv->out_off += 8;
}

static void out_string(sftp_server_t *srv, const char *s, uint32_t len) {
    put_u32(srv->out_buf + srv->out_off, len);
    srv->out_off += 4;
    if (len > 0) {
        memcpy(srv->out_buf + srv->out_off, s, len);
        srv->out_off += len;
    }
}

static void out_cstring(sftp_server_t *srv, const char *s) {
    out_string(srv, s, (uint32_t)strlen(s));
}

static void out_data(sftp_server_t *srv, const uint8_t *data, uint32_t len) {
    put_u32(srv->out_buf + srv->out_off, len);
    srv->out_off += 4;
    if (len > 0) {
        memcpy(srv->out_buf + srv->out_off, data, len);
        srv->out_off += len;
    }
}

static int out_send(sftp_server_t *srv) {
    uint32_t pkt_len = (uint32_t)(srv->out_off - 4);

    put_u32(srv->out_buf, pkt_len);
    if (!srv->io.emit) {
        errno = EINVAL;
        return -1;
    }
    return srv->io.emit(srv->io.user, srv->out_buf, srv->out_off);
}

static void encode_attrs(sftp_server_t *srv, const struct stat *st) {
    uint32_t flags = SSH_FILEXFER_ATTR_SIZE |
                     SSH_FILEXFER_ATTR_UIDGID |
                     SSH_FILEXFER_ATTR_PERMISSIONS |
                     SSH_FILEXFER_ATTR_ACMODTIME;
    uint32_t mode = (uint32_t)st->st_mode;

    out_u32(srv, flags);
    out_u64(srv, (uint64_t)st->st_size);
    out_u32(srv, (uint32_t)st->st_uid);
    out_u32(srv, (uint32_t)st->st_gid);
    if ((st->st_mode & 0040000) != 0)
        mode |= 0040000;
    out_u32(srv, mode);
    out_u32(srv, (uint32_t)st->st_atime);
    out_u32(srv, (uint32_t)st->st_mtime);
}

static void encode_attrs_empty(sftp_server_t *srv) {
    out_u32(srv, 0);
}

static uint32_t parse_attrs_mode(const uint8_t *buf, size_t buf_len, size_t *off) {
    uint32_t flags;
    uint32_t mode = 0755;

    if (get_uint32(buf, buf_len, off, &flags) < 0)
        return mode;
    if (flags & SSH_FILEXFER_ATTR_SIZE) {
        uint64_t dummy64;
        get_uint64(buf, buf_len, off, &dummy64);
    }
    if (flags & SSH_FILEXFER_ATTR_UIDGID) {
        uint32_t dummy32;
        get_uint32(buf, buf_len, off, &dummy32);
        get_uint32(buf, buf_len, off, &dummy32);
    }
    if (flags & SSH_FILEXFER_ATTR_PERMISSIONS)
        get_uint32(buf, buf_len, off, &mode);
    return mode;
}

static mode_t infer_type_bits(mode_t current_mode, mode_t fallback_type) {
    mode_t type = current_mode & S_IFMT;

    if (type == 0 && (current_mode & 0040000) != 0)
        type = S_IFDIR;
    if (type == 0)
        type = fallback_type;
    return type;
}

static mode_t merge_permissions_preserve_type(mode_t current_mode, uint32_t mode,
        mode_t fallback_type) {
    return infer_type_bits(current_mode, fallback_type) | (mode & 0777);
}

static int chmod_preserve_type(const char *path, uint32_t mode,
        mode_t fallback_type) {
    struct stat st;

    if (stat(path, &st) < 0)
        return -1;
    return chmod(path, merge_permissions_preserve_type(st.st_mode, mode,
                fallback_type));
}

static int fchmod_preserve_type(int fd, uint32_t mode, mode_t fallback_type) {
    struct stat st;

    if (fstat(fd, &st) < 0)
        return -1;
    return fchmod(fd, merge_permissions_preserve_type(st.st_mode, mode,
                fallback_type));
}

static void reset_writeback(handle_t *h) {
    if (!h)
        return;
    free(h->wb_buf);
    h->wb_buf = NULL;
    h->wb_cap = 0;
    h->wb_len = 0;
    h->wb_off = 0;
}

static int ensure_writeback_capacity(handle_t *h, size_t need) {
    uint8_t *buf;
    size_t cap;

    if (!h)
        return -1;
    if (need <= h->wb_cap)
        return 0;
    cap = WRITEBACK_BUFFER_SIZE;
    while (cap < need)
        cap *= 2;
    buf = (uint8_t *)realloc(h->wb_buf, cap);
    if (!buf)
        return -1;
    h->wb_buf = buf;
    h->wb_cap = cap;
    return 0;
}

static int flush_handle_writeback(handle_t *h) {
    size_t total = 0;

    if (!h || h->type != HANDLE_FILE || h->wb_len == 0)
        return 0;
    if (lseek(h->fd, (off_t)h->wb_off, SEEK_SET) < 0)
        return -1;
    while (total < h->wb_len) {
        ssize_t w = write(h->fd, h->wb_buf + total, h->wb_len - total);
        if (w < 0) {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            h->wb_len = 0;
            return -1;
        }
        if (w == 0) {
            errno = EIO;
            h->wb_len = 0;
            return -1;
        }
        total += (size_t)w;
    }
    h->wb_len = 0;
    return 0;
}

static int flush_handle_idx(sftp_server_t *srv, int idx) {
    if (!srv || idx < 0 || idx >= MAX_HANDLES)
        return -1;
    return flush_handle_writeback(&srv->handles[idx]);
}

static int alloc_handle_file(sftp_server_t *srv, int fd, const char *path) {
    int i;

    for (i = 0; i < MAX_HANDLES; i++) {
        if (srv->handles[i].type == 0) {
            srv->handles[i].type = HANDLE_FILE;
            srv->handles[i].fd = fd;
            srv->handles[i].dirp = NULL;
            strncpy(srv->handles[i].path, path ? path : "",
                    sizeof(srv->handles[i].path) - 1);
            srv->handles[i].path[sizeof(srv->handles[i].path) - 1] = '\0';
            srv->handles[i].wb_buf = NULL;
            srv->handles[i].wb_cap = 0;
            srv->handles[i].wb_len = 0;
            srv->handles[i].wb_off = 0;
            return i;
        }
    }
    return -1;
}

static int alloc_handle_dir(sftp_server_t *srv, DIR *dirp, const char *path) {
    int i;

    for (i = 0; i < MAX_HANDLES; i++) {
        if (srv->handles[i].type == 0) {
            srv->handles[i].type = HANDLE_DIR;
            srv->handles[i].fd = -1;
            srv->handles[i].dirp = dirp;
            strncpy(srv->handles[i].path, path ? path : "",
                    sizeof(srv->handles[i].path) - 1);
            srv->handles[i].path[sizeof(srv->handles[i].path) - 1] = '\0';
            srv->handles[i].wb_buf = NULL;
            srv->handles[i].wb_cap = 0;
            srv->handles[i].wb_len = 0;
            srv->handles[i].wb_off = 0;
            return i;
        }
    }
    return -1;
}

static void free_handle(sftp_server_t *srv, int idx) {
    if (!srv || idx < 0 || idx >= MAX_HANDLES)
        return;
    if (srv->handles[idx].type == HANDLE_FILE && srv->handles[idx].fd >= 0)
        (void)flush_handle_writeback(&srv->handles[idx]);
    if (srv->handles[idx].type == HANDLE_FILE && srv->handles[idx].fd >= 0)
        close(srv->handles[idx].fd);
    if (srv->handles[idx].type == HANDLE_DIR && srv->handles[idx].dirp)
        closedir(srv->handles[idx].dirp);
    reset_writeback(&srv->handles[idx]);
    srv->handles[idx].type = 0;
    srv->handles[idx].fd = -1;
    srv->handles[idx].dirp = NULL;
}

static int get_handle_from_pkt(sftp_server_t *srv, const uint8_t *buf,
        size_t buf_len, size_t *off) {
    uint32_t hlen;
    uint32_t idx;
    const char *hstr = get_string(buf, buf_len, off, &hlen);

    if (!srv || !hstr || hlen != 4)
        return -1;
    idx = get_u32((const uint8_t *)hstr);
    if (idx >= MAX_HANDLES)
        return -1;
    if (srv->handles[idx].type == 0)
        return -1;
    return (int)idx;
}

static int send_status(sftp_server_t *srv, uint32_t id, uint32_t code,
        const char *msg) {
    out_begin(srv, SSH_FXP_STATUS);
    out_u32(srv, id);
    out_u32(srv, code);
    out_cstring(srv, msg ? msg : "");
    out_cstring(srv, "");
    return out_send(srv);
}

static int send_handle(sftp_server_t *srv, uint32_t id, int handle_idx) {
    uint8_t hbuf[4];

    put_u32(hbuf, (uint32_t)handle_idx);
    out_begin(srv, SSH_FXP_HANDLE);
    out_u32(srv, id);
    out_data(srv, hbuf, 4);
    return out_send(srv);
}

static int send_extended_limits(sftp_server_t *srv, uint32_t id) {
    out_begin(srv, SSH_FXP_EXTENDED_REPLY);
    out_u32(srv, id);
    out_u64(srv, SFTP_LIMITS_MAX_PACKET);
    out_u64(srv, SFTP_LIMITS_MAX_READ);
    out_u64(srv, SFTP_LIMITS_MAX_WRITE);
    out_u64(srv, SFTP_LIMITS_MAX_OPEN);
    return out_send(srv);
}

static int send_version(sftp_server_t *srv) {
    out_begin(srv, SSH_FXP_VERSION);
    out_u32(srv, SFTP_VERSION);
    out_cstring(srv, SFTP_EXT_LIMITS);
    out_cstring(srv, SFTP_EXT_LIMITS_VER);
    return out_send(srv);
}

static uint32_t errno_to_status(void) {
    switch (errno) {
    case ENOENT:
        return SSH_FX_NO_SUCH_FILE;
    case EACCES:
    case EPERM:
        return SSH_FX_PERMISSION_DENIED;
    default:
        return SSH_FX_FAILURE;
    }
}

static int handle_open(sftp_server_t *srv, const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id, pflags, slen;
    uint32_t mode;
    int flags = 0;
    char pathbuf[512];
    struct stat st;
    int st_rc;
    int st_errno;
    int open_flags;
    fsinfo_t finfo;
    int fd;
    int h;

    if (get_uint32(buf, len, &off, &id) < 0)
        return -1;
    {
        const char *path = get_string(buf, len, &off, &slen);
        if (!path)
            return send_status(srv, id, SSH_FX_BAD_MESSAGE, "bad open");
        if (get_uint32(buf, len, &off, &pflags) < 0)
            return send_status(srv, id, SSH_FX_BAD_MESSAGE, "bad open flags");
        mode = parse_attrs_mode(buf, len, &off);
        if ((pflags & SSH_FXF_READ) && (pflags & SSH_FXF_WRITE))
            flags = O_RDWR;
        else if (pflags & SSH_FXF_WRITE)
            flags = O_WRONLY;
        else
            flags = O_RDONLY;
        if (pflags & SSH_FXF_CREAT)
            flags |= O_CREAT;
        if (pflags & SSH_FXF_TRUNC)
            flags |= O_TRUNC;
        if (pflags & SSH_FXF_APPEND)
            flags |= O_APPEND;
        if (slen >= sizeof(pathbuf))
            return send_status(srv, id, SSH_FX_FAILURE, "path too long");
        memcpy(pathbuf, path, slen);
        pathbuf[slen] = '\0';
    }

    errno = 0;
    st_rc = stat(pathbuf, &st);
    st_errno = errno;
    if (st_rc == 0 && S_ISDIR(st.st_mode))
        return send_status(srv, id, SSH_FX_FAILURE, "is a directory");
    if (st_rc == 0 && (pflags & SSH_FXF_CREAT) && (pflags & SSH_FXF_EXCL))
        return send_status(srv, id, SSH_FX_FAILURE, "file exists");

    if (st_rc < 0 && (pflags & SSH_FXF_CREAT) &&
            (st_errno == 0 || st_errno == ENOENT)) {
        int create_mode = (int)(mode & 0777);
        if (create_mode == 0)
            create_mode = 0644;
        if (vfs_create(pathbuf, NULL, FS_TYPE_FILE, create_mode,
                    false, false) != 0) {
            int err = errno ? errno : EIO;
            errno = err;
            return send_status(srv, id, errno_to_status(), strerror(err));
        }
    }

    open_flags = flags & ~O_CREAT;
    if (vfs_get_by_name(pathbuf, &finfo) != 0) {
        int err = errno ? errno : ENOENT;
        errno = err;
        return send_status(srv, id, errno_to_status(), strerror(err));
    }
    fd = vfs_open(&finfo, open_flags);
    if (fd < 0) {
        int err = errno ? errno : EIO;
        errno = err;
        return send_status(srv, id, errno_to_status(), strerror(err));
    }
    if ((pflags & SSH_FXF_CREAT) &&
            fchmod_preserve_type(fd, mode, S_IFREG) < 0) {
        int err = errno ? errno : EIO;
        close(fd);
        errno = err;
        return send_status(srv, id, errno_to_status(), strerror(err));
    }

    h = alloc_handle_file(srv, fd, pathbuf);
    if (h < 0) {
        close(fd);
        return send_status(srv, id, SSH_FX_FAILURE, "too many handles");
    }
    return send_handle(srv, id, h);
}

static int handle_close(sftp_server_t *srv, const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id;
    int h;
    int status = SSH_FX_OK;
    const char *msg = "";

    if (get_uint32(buf, len, &off, &id) < 0)
        return -1;
    h = get_handle_from_pkt(srv, buf, len, &off);
    if (h < 0)
        return send_status(srv, id, SSH_FX_FAILURE, "invalid handle");
    if (srv->handles[h].type == HANDLE_FILE && flush_handle_idx(srv, h) < 0) {
        status = errno_to_status();
        msg = strerror(errno);
    }
    free_handle(srv, h);
    return send_status(srv, id, status, msg);
}

static int handle_read(sftp_server_t *srv, const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id, rlen;
    uint64_t roff;
    int h;
    uint8_t *data;
    ssize_t n;

    if (get_uint32(buf, len, &off, &id) < 0)
        return -1;
    h = get_handle_from_pkt(srv, buf, len, &off);
    if (h < 0)
        return send_status(srv, id, SSH_FX_FAILURE, "invalid handle");
    if (get_uint64(buf, len, &off, &roff) < 0)
        return send_status(srv, id, SSH_FX_BAD_MESSAGE, "");
    if (get_uint32(buf, len, &off, &rlen) < 0)
        return send_status(srv, id, SSH_FX_BAD_MESSAGE, "");
    if (srv->handles[h].type != HANDLE_FILE)
        return send_status(srv, id, SSH_FX_FAILURE, "not a file handle");
    if (flush_handle_idx(srv, h) < 0)
        return send_status(srv, id, SSH_FX_FAILURE, strerror(errno));
    if (rlen > MAX_READ_REPLY_DATA)
        rlen = MAX_READ_REPLY_DATA;
    if (lseek(srv->handles[h].fd, (off_t)roff, SEEK_SET) < 0)
        return send_status(srv, id, SSH_FX_FAILURE, "seek failed");

    data = (uint8_t *)malloc(rlen);
    if (!data)
        return send_status(srv, id, SSH_FX_FAILURE, "out of memory");
    n = read(srv->handles[h].fd, data, rlen);
    if (n < 0) {
        free(data);
        return send_status(srv, id, SSH_FX_FAILURE, strerror(errno));
    }
    if (n == 0) {
        free(data);
        return send_status(srv, id, SSH_FX_EOF, "");
    }

    out_begin(srv, SSH_FXP_DATA);
    out_u32(srv, id);
    out_data(srv, data, (uint32_t)n);
    free(data);
    return out_send(srv);
}

static int handle_write(sftp_server_t *srv, const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id, dlen;
    uint64_t woff;
    int h;
    const char *data;
    handle_t *handle;

    if (get_uint32(buf, len, &off, &id) < 0)
        return -1;
    h = get_handle_from_pkt(srv, buf, len, &off);
    if (h < 0)
        return send_status(srv, id, SSH_FX_FAILURE, "invalid handle");
    if (get_uint64(buf, len, &off, &woff) < 0)
        return send_status(srv, id, SSH_FX_BAD_MESSAGE, "");
    data = get_string(buf, len, &off, &dlen);
    if (!data)
        return send_status(srv, id, SSH_FX_BAD_MESSAGE, "");
    if (srv->handles[h].type != HANDLE_FILE)
        return send_status(srv, id, SSH_FX_FAILURE, "not a file handle");

    handle = &srv->handles[h];
    if (handle->wb_len > 0) {
        uint64_t expected = handle->wb_off + (uint64_t)handle->wb_len;
        if (woff != expected || handle->wb_len + dlen > handle->wb_cap) {
            if (flush_handle_writeback(handle) < 0)
                return send_status(srv, id, SSH_FX_FAILURE, strerror(errno));
        }
    }
    if (handle->wb_len == 0) {
        if (ensure_writeback_capacity(handle, dlen) < 0) {
            int err = errno ? errno : ENOMEM;
            errno = err;
            return send_status(srv, id, SSH_FX_FAILURE, strerror(err));
        }
        handle->wb_off = woff;
    }

    if (handle->wb_len + dlen <= handle->wb_cap) {
        memcpy(handle->wb_buf + handle->wb_len, data, dlen);
        handle->wb_len += dlen;
        return send_status(srv, id, SSH_FX_OK, "");
    }

    if (flush_handle_writeback(handle) < 0)
        return send_status(srv, id, SSH_FX_FAILURE, strerror(errno));
    if (lseek(handle->fd, (off_t)woff, SEEK_SET) < 0)
        return send_status(srv, id, SSH_FX_FAILURE, "seek failed");
    {
        size_t total = 0;
        while (total < dlen) {
            ssize_t w = write(handle->fd, data + total, dlen - total);
            if (w <= 0) {
                int err = errno;
                if (w < 0 && (err == EINTR || err == EAGAIN))
                    continue;
                if (err == 0)
                    err = EIO;
                errno = err;
                return send_status(srv, id, SSH_FX_FAILURE, strerror(err));
            }
            total += (size_t)w;
        }
    }
    return send_status(srv, id, SSH_FX_OK, "");
}

static int handle_stat_common(sftp_server_t *srv, const uint8_t *buf,
        size_t len) {
    size_t off = 0;
    uint32_t id, slen;
    const char *path;
    char pathbuf[512];
    struct stat st;

    if (get_uint32(buf, len, &off, &id) < 0)
        return -1;
    path = get_string(buf, len, &off, &slen);
    if (!path)
        return send_status(srv, id, SSH_FX_BAD_MESSAGE, "");
    if (slen >= sizeof(pathbuf))
        return send_status(srv, id, SSH_FX_FAILURE, "path too long");
    memcpy(pathbuf, path, slen);
    pathbuf[slen] = '\0';
    if (stat(pathbuf, &st) < 0)
        return send_status(srv, id, errno_to_status(), strerror(errno));
    out_begin(srv, SSH_FXP_ATTRS);
    out_u32(srv, id);
    encode_attrs(srv, &st);
    return out_send(srv);
}

static int handle_fstat(sftp_server_t *srv, const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id;
    int h;
    struct stat st;

    if (get_uint32(buf, len, &off, &id) < 0)
        return -1;
    h = get_handle_from_pkt(srv, buf, len, &off);
    if (h < 0)
        return send_status(srv, id, SSH_FX_FAILURE, "invalid handle");
    if (srv->handles[h].type == HANDLE_DIR) {
        if (stat(srv->handles[h].path, &st) < 0)
            return send_status(srv, id, errno_to_status(), strerror(errno));
        out_begin(srv, SSH_FXP_ATTRS);
        out_u32(srv, id);
        encode_attrs(srv, &st);
        return out_send(srv);
    }
    if (flush_handle_idx(srv, h) < 0)
        return send_status(srv, id, errno_to_status(), strerror(errno));
    if (fstat(srv->handles[h].fd, &st) < 0)
        return send_status(srv, id, errno_to_status(), strerror(errno));
    out_begin(srv, SSH_FXP_ATTRS);
    out_u32(srv, id);
    encode_attrs(srv, &st);
    return out_send(srv);
}

static int handle_setstat(sftp_server_t *srv, const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id, slen;
    const char *path;
    char pathbuf[512];
    uint32_t mode;

    if (get_uint32(buf, len, &off, &id) < 0)
        return -1;
    path = get_string(buf, len, &off, &slen);
    if (!path)
        return send_status(srv, id, SSH_FX_BAD_MESSAGE, "");
    if (slen >= sizeof(pathbuf))
        return send_status(srv, id, SSH_FX_FAILURE, "path too long");
    memcpy(pathbuf, path, slen);
    pathbuf[slen] = '\0';
    mode = parse_attrs_mode(buf, len, &off);
    if (chmod_preserve_type(pathbuf, mode, S_IFREG) < 0)
        return send_status(srv, id, errno_to_status(), strerror(errno));
    return send_status(srv, id, SSH_FX_OK, "");
}

static int handle_fsetstat(sftp_server_t *srv, const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id;
    int h;
    uint32_t mode;

    if (get_uint32(buf, len, &off, &id) < 0)
        return -1;
    h = get_handle_from_pkt(srv, buf, len, &off);
    if (h < 0)
        return send_status(srv, id, SSH_FX_FAILURE, "invalid handle");
    if (srv->handles[h].type == HANDLE_FILE && flush_handle_idx(srv, h) < 0)
        return send_status(srv, id, errno_to_status(), strerror(errno));
    mode = parse_attrs_mode(buf, len, &off);
    if (srv->handles[h].type == HANDLE_FILE &&
            srv->handles[h].fd >= 0 &&
            fchmod_preserve_type(srv->handles[h].fd, mode, S_IFREG) < 0)
        return send_status(srv, id, errno_to_status(), strerror(errno));
    return send_status(srv, id, SSH_FX_OK, "");
}

static int handle_opendir(sftp_server_t *srv, const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id, slen;
    const char *path;
    char pathbuf[512];
    DIR *dirp;
    int h;

    if (get_uint32(buf, len, &off, &id) < 0)
        return -1;
    path = get_string(buf, len, &off, &slen);
    if (!path)
        return send_status(srv, id, SSH_FX_BAD_MESSAGE, "");
    if (slen >= sizeof(pathbuf))
        return send_status(srv, id, SSH_FX_FAILURE, "path too long");
    memcpy(pathbuf, path, slen);
    pathbuf[slen] = '\0';
    dirp = opendir(pathbuf);
    if (!dirp)
        return send_status(srv, id, errno_to_status(), strerror(errno));
    h = alloc_handle_dir(srv, dirp, pathbuf);
    if (h < 0) {
        closedir(dirp);
        return send_status(srv, id, SSH_FX_FAILURE, "too many handles");
    }
    return send_handle(srv, id, h);
}

static int handle_readdir(sftp_server_t *srv, const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id;
    int h;
    struct dirent *ent;
    int count = 0;
    enum { MAX_DIR_ENTRIES = 32 };
    struct {
        char name[256];
        struct stat st;
        int valid;
    } entries[MAX_DIR_ENTRIES];

    if (get_uint32(buf, len, &off, &id) < 0)
        return -1;
    h = get_handle_from_pkt(srv, buf, len, &off);
    if (h < 0)
        return send_status(srv, id, SSH_FX_FAILURE, "invalid handle");
    if (srv->handles[h].type != HANDLE_DIR || !srv->handles[h].dirp)
        return send_status(srv, id, SSH_FX_FAILURE, "not a directory handle");

    while (count < MAX_DIR_ENTRIES &&
            (ent = readdir(srv->handles[h].dirp)) != NULL) {
        char fullpath[512];
        size_t dlen2;

        strncpy(entries[count].name, ent->d_name,
                sizeof(entries[count].name) - 1);
        entries[count].name[sizeof(entries[count].name) - 1] = '\0';
        dlen2 = strlen(srv->handles[h].path);
        if (dlen2 > 0 && srv->handles[h].path[dlen2 - 1] == '/')
            snprintf(fullpath, sizeof(fullpath), "%s%s",
                    srv->handles[h].path, ent->d_name);
        else
            snprintf(fullpath, sizeof(fullpath), "%s/%s",
                    srv->handles[h].path, ent->d_name);
        if (stat(fullpath, &entries[count].st) == 0)
            entries[count].valid = 1;
        else {
            memset(&entries[count].st, 0, sizeof(struct stat));
            entries[count].valid = 0;
        }
        count++;
    }

    if (count == 0)
        return send_status(srv, id, SSH_FX_EOF, "");

    out_begin(srv, SSH_FXP_NAME);
    out_u32(srv, id);
    out_u32(srv, (uint32_t)count);
    {
        int i;
        for (i = 0; i < count; i++) {
            char longname[512];

            out_cstring(srv, entries[i].name);
            if (entries[i].valid) {
                uint32_t m = (uint32_t)entries[i].st.st_mode;
                char type_c = '-';
                if (m & 0040000)
                    type_c = 'd';
                snprintf(longname, sizeof(longname),
                        "%c%c%c%c%c%c%c%c%c%c %u %u %u %lu %s",
                        type_c,
                        (m & 0400) ? 'r' : '-', (m & 0200) ? 'w' : '-',
                        (m & 0100) ? 'x' : '-', (m & 040) ? 'r' : '-',
                        (m & 020) ? 'w' : '-', (m & 010) ? 'x' : '-',
                        (m & 04) ? 'r' : '-', (m & 02) ? 'w' : '-',
                        (m & 01) ? 'x' : '-',
                        (unsigned)entries[i].st.st_uid,
                        (unsigned)entries[i].st.st_gid,
                        (unsigned)entries[i].st.st_size,
                        (unsigned long)entries[i].st.st_mtime,
                        entries[i].name);
            } else {
                snprintf(longname, sizeof(longname),
                        "---------- 0 0 0 0 %s", entries[i].name);
            }
            out_cstring(srv, longname);
            if (entries[i].valid)
                encode_attrs(srv, &entries[i].st);
            else
                encode_attrs_empty(srv);
        }
    }
    return out_send(srv);
}

static int handle_remove(sftp_server_t *srv, const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id, slen;
    const char *path;
    char pathbuf[512];

    if (get_uint32(buf, len, &off, &id) < 0)
        return -1;
    path = get_string(buf, len, &off, &slen);
    if (!path)
        return send_status(srv, id, SSH_FX_BAD_MESSAGE, "");
    if (slen >= sizeof(pathbuf))
        return send_status(srv, id, SSH_FX_FAILURE, "path too long");
    memcpy(pathbuf, path, slen);
    pathbuf[slen] = '\0';
    if (unlink(pathbuf) < 0)
        return send_status(srv, id, errno_to_status(), strerror(errno));
    return send_status(srv, id, SSH_FX_OK, "");
}

static int handle_mkdir(sftp_server_t *srv, const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id, slen;
    const char *path;
    char pathbuf[512];
    uint32_t mode;

    if (get_uint32(buf, len, &off, &id) < 0)
        return -1;
    path = get_string(buf, len, &off, &slen);
    if (!path)
        return send_status(srv, id, SSH_FX_BAD_MESSAGE, "");
    if (slen >= sizeof(pathbuf))
        return send_status(srv, id, SSH_FX_FAILURE, "path too long");
    memcpy(pathbuf, path, slen);
    pathbuf[slen] = '\0';
    mode = parse_attrs_mode(buf, len, &off);
    if (mkdir(pathbuf, mode & 0777) < 0)
        return send_status(srv, id, errno_to_status(), strerror(errno));
    return send_status(srv, id, SSH_FX_OK, "");
}

static int handle_rmdir(sftp_server_t *srv, const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id, slen;
    const char *path;
    char pathbuf[512];

    if (get_uint32(buf, len, &off, &id) < 0)
        return -1;
    path = get_string(buf, len, &off, &slen);
    if (!path)
        return send_status(srv, id, SSH_FX_BAD_MESSAGE, "");
    if (slen >= sizeof(pathbuf))
        return send_status(srv, id, SSH_FX_FAILURE, "path too long");
    memcpy(pathbuf, path, slen);
    pathbuf[slen] = '\0';
    if (rmdir(pathbuf) < 0)
        return send_status(srv, id, errno_to_status(), strerror(errno));
    return send_status(srv, id, SSH_FX_OK, "");
}

static int handle_realpath(sftp_server_t *srv, const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id, slen;
    const char *path;
    char pathbuf[512];
    char resolved[512];
    size_t rlen;

    if (get_uint32(buf, len, &off, &id) < 0)
        return -1;
    path = get_string(buf, len, &off, &slen);
    if (!path)
        return send_status(srv, id, SSH_FX_BAD_MESSAGE, "");
    if (slen >= sizeof(pathbuf))
        return send_status(srv, id, SSH_FX_FAILURE, "path too long");
    memcpy(pathbuf, path, slen);
    pathbuf[slen] = '\0';

    if (pathbuf[0] == '/') {
        strncpy(resolved, pathbuf, sizeof(resolved) - 1);
        resolved[sizeof(resolved) - 1] = '\0';
    } else if (pathbuf[0] == '.' && (pathbuf[1] == '\0' || pathbuf[1] == '/')) {
        char cwd[512];
        if (getcwd(cwd, sizeof(cwd)) == NULL)
            strncpy(cwd, "/", sizeof(cwd));
        if (pathbuf[1] == '/' && pathbuf[2] != '\0')
            snprintf(resolved, sizeof(resolved), "%s/%s", cwd, pathbuf + 2);
        else
            strncpy(resolved, cwd, sizeof(resolved) - 1);
        resolved[sizeof(resolved) - 1] = '\0';
    } else {
        char cwd[512];
        if (getcwd(cwd, sizeof(cwd)) == NULL)
            strncpy(cwd, "/", sizeof(cwd));
        snprintf(resolved, sizeof(resolved), "%s/%s", cwd, pathbuf);
    }

    rlen = strlen(resolved);
    while (rlen > 1 && resolved[rlen - 1] == '/') {
        resolved[--rlen] = '\0';
    }

    out_begin(srv, SSH_FXP_NAME);
    out_u32(srv, id);
    out_u32(srv, 1);
    out_cstring(srv, resolved);
    out_cstring(srv, resolved);
    encode_attrs_empty(srv);
    return out_send(srv);
}

static int handle_rename(sftp_server_t *srv, const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id, slen1, slen2;
    const char *oldpath;
    const char *newpath;
    char old_buf[512], new_buf[512];

    if (get_uint32(buf, len, &off, &id) < 0)
        return -1;
    oldpath = get_string(buf, len, &off, &slen1);
    if (!oldpath)
        return send_status(srv, id, SSH_FX_BAD_MESSAGE, "");
    newpath = get_string(buf, len, &off, &slen2);
    if (!newpath)
        return send_status(srv, id, SSH_FX_BAD_MESSAGE, "");
    if (slen1 >= sizeof(old_buf) || slen2 >= sizeof(new_buf))
        return send_status(srv, id, SSH_FX_FAILURE, "path too long");
    memcpy(old_buf, oldpath, slen1);
    old_buf[slen1] = '\0';
    memcpy(new_buf, newpath, slen2);
    new_buf[slen2] = '\0';
    if (rename(old_buf, new_buf) < 0)
        return send_status(srv, id, errno_to_status(), strerror(errno));
    return send_status(srv, id, SSH_FX_OK, "");
}

static int handle_extended(sftp_server_t *srv, const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id, ext_len;
    const char *ext_name;

    if (get_uint32(buf, len, &off, &id) < 0)
        return -1;
    ext_name = get_string(buf, len, &off, &ext_len);
    if (!ext_name)
        return send_status(srv, id, SSH_FX_BAD_MESSAGE, "");
    if (ext_len == strlen(SFTP_EXT_LIMITS) &&
            memcmp(ext_name, SFTP_EXT_LIMITS, ext_len) == 0)
        return send_extended_limits(srv, id);
    return send_status(srv, id, SSH_FX_OP_UNSUPPORTED,
            "unsupported extension");
}

static int process_packet(sftp_server_t *srv, uint8_t type,
        const uint8_t *payload, size_t payload_len) {
    switch (type) {
    case SSH_FXP_OPEN:
        return handle_open(srv, payload, payload_len);
    case SSH_FXP_CLOSE:
        return handle_close(srv, payload, payload_len);
    case SSH_FXP_READ:
        return handle_read(srv, payload, payload_len);
    case SSH_FXP_WRITE:
        return handle_write(srv, payload, payload_len);
    case SSH_FXP_LSTAT:
    case SSH_FXP_STAT:
        return handle_stat_common(srv, payload, payload_len);
    case SSH_FXP_FSTAT:
        return handle_fstat(srv, payload, payload_len);
    case SSH_FXP_SETSTAT:
        return handle_setstat(srv, payload, payload_len);
    case SSH_FXP_FSETSTAT:
        return handle_fsetstat(srv, payload, payload_len);
    case SSH_FXP_OPENDIR:
        return handle_opendir(srv, payload, payload_len);
    case SSH_FXP_READDIR:
        return handle_readdir(srv, payload, payload_len);
    case SSH_FXP_REMOVE:
        return handle_remove(srv, payload, payload_len);
    case SSH_FXP_MKDIR:
        return handle_mkdir(srv, payload, payload_len);
    case SSH_FXP_RMDIR:
        return handle_rmdir(srv, payload, payload_len);
    case SSH_FXP_REALPATH:
        return handle_realpath(srv, payload, payload_len);
    case SSH_FXP_RENAME:
        return handle_rename(srv, payload, payload_len);
    case SSH_FXP_EXTENDED:
        return handle_extended(srv, payload, payload_len);
    case SSH_FXP_READLINK:
    case SSH_FXP_SYMLINK: {
        uint32_t id = 0;
        if (payload_len >= 4)
            id = get_u32(payload);
        return send_status(srv, id, SSH_FX_OP_UNSUPPORTED, "not supported");
    }
    default: {
        uint32_t id = 0;
        if (payload_len >= 4)
            id = get_u32(payload);
        return send_status(srv, id, SSH_FX_OP_UNSUPPORTED, "unknown request");
    }
    }
}

static int append_input(sftp_server_t *srv, const uint8_t *data, size_t len) {
    uint8_t *new_buf;
    size_t new_cap;

    if (len == 0)
        return 0;
    if (srv->rx_len + len <= srv->rx_cap) {
        memcpy(srv->rx_buf + srv->rx_len, data, len);
        srv->rx_len += len;
        return 0;
    }
    new_cap = srv->rx_cap ? srv->rx_cap : 4096;
    while (new_cap < srv->rx_len + len)
        new_cap *= 2;
    new_buf = (uint8_t *)realloc(srv->rx_buf, new_cap);
    if (!new_buf)
        return -1;
    srv->rx_buf = new_buf;
    srv->rx_cap = new_cap;
    memcpy(srv->rx_buf + srv->rx_len, data, len);
    srv->rx_len += len;
    return 0;
}

sftp_server_t *sftp_server_create(const sftp_server_io_t *io) {
    sftp_server_t *srv = (sftp_server_t *)calloc(1, sizeof(*srv));

    if (!srv)
        return NULL;
    if (io)
        srv->io = *io;
    return srv;
}

void sftp_server_destroy(sftp_server_t *srv) {
    int i;

    if (!srv)
        return;
    for (i = 0; i < MAX_HANDLES; i++)
        free_handle(srv, i);
    free(srv->rx_buf);
    free(srv);
}

int sftp_server_feed(sftp_server_t *srv, const uint8_t *data, size_t len) {
    if (!srv || (!data && len > 0)) {
        errno = EINVAL;
        return -1;
    }
    if (append_input(srv, data, len) < 0)
        return -1;

    while (srv->rx_len >= 4) {
        uint32_t pkt_len = get_u32(srv->rx_buf);
        uint8_t type;
        const uint8_t *payload;
        size_t frame_len;

        if (pkt_len == 0 || pkt_len > MAX_PKT_SIZE) {
            errno = EINVAL;
            return -1;
        }
        frame_len = (size_t)pkt_len + 4;
        if (srv->rx_len < frame_len)
            break;

        type = srv->rx_buf[4];
        payload = srv->rx_buf + 5;
        if (type == SSH_FXP_INIT) {
            if (send_version(srv) < 0)
                return -1;
        } else if (process_packet(srv, type, payload, pkt_len - 1) < 0) {
            return -1;
        }

        if (srv->rx_len > frame_len)
            memmove(srv->rx_buf, srv->rx_buf + frame_len,
                    srv->rx_len - frame_len);
        srv->rx_len -= frame_len;
    }
    return 0;
}
