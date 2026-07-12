/*
 * sftp-server - SFTP (SSH File Transfer Protocol) server
 *
 * Implements SFTP protocol version 3 over stdin/stdout.
 * Invoked by sshd via the "subsystem" channel request.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

/* SFTP Protocol Version */
#define SFTP_VERSION 3

/* SFTP Packet Types */
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

/* SFTP Status Codes */
#define SSH_FX_OK               0
#define SSH_FX_EOF              1
#define SSH_FX_NO_SUCH_FILE     2
#define SSH_FX_PERMISSION_DENIED 3
#define SSH_FX_FAILURE          4
#define SSH_FX_BAD_MESSAGE      5
#define SSH_FX_OP_UNSUPPORTED   8

/* SFTP Attribute Flags */
#define SSH_FILEXFER_ATTR_SIZE        0x00000001
#define SSH_FILEXFER_ATTR_UIDGID      0x00000002
#define SSH_FILEXFER_ATTR_PERMISSIONS 0x00000004
#define SSH_FILEXFER_ATTR_ACMODTIME   0x00000008

/* SFTP Open Flags */
#define SSH_FXF_READ    0x00000001
#define SSH_FXF_WRITE   0x00000002
#define SSH_FXF_APPEND  0x00000004
#define SSH_FXF_CREAT   0x00000008
#define SSH_FXF_TRUNC   0x00000010
#define SSH_FXF_EXCL    0x00000020

/* Handle management */
#define MAX_HANDLES 64
#define HANDLE_FILE 1
#define HANDLE_DIR  2

typedef struct {
    int type;       /* 0=unused, HANDLE_FILE, HANDLE_DIR */
    int fd;         /* file descriptor for file handles */
    DIR *dirp;      /* directory pointer for dir handles */
    char path[512]; /* path for directory reads */
} handle_t;

static handle_t handles[MAX_HANDLES];

/* Packet buffer */
#define MAX_PKT_SIZE (256 * 1024)
/*
 * Keep each SFTP DATA reply comfortably below small SSH channel windows.
 * Some clients advertise a 32 KiB-256 KiB window and only send WINDOW_ADJUST
 * after they have received a complete SFTP packet. If we generate a huge
 * single DATA packet, sshd can fill the channel window mid-packet and both
 * sides deadlock waiting for the other to make progress.
 */
#define MAX_READ_REPLY_DATA (16 * 1024)
static uint8_t pkt_buf[MAX_PKT_SIZE];
static uint8_t out_buf[MAX_PKT_SIZE];

/* ---- Low-level I/O ---- */

static int read_exact(void *buf, size_t n) {
    uint8_t *p = (uint8_t *)buf;
    size_t total = 0;
    while (total < n) {
        ssize_t r = read(STDIN_FILENO, p + total, n - total);
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) return -1;
        total += (size_t)r;
    }
    return 0;
}

static int write_all(const void *buf, size_t n) {
    const uint8_t *p = (const uint8_t *)buf;
    size_t total = 0;
    while (total < n) {
        ssize_t w = write(STDOUT_FILENO, p + total, n - total);
        if (w < 0) {
            if (errno == EINTR || errno == EAGAIN) continue;
            return -1;
        }
        total += (size_t)w;
    }
    return 0;
}

/* ---- Encoding helpers ---- */

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
    put_u32(buf + 4, (uint32_t)(val & 0xffffffff));
}

static uint64_t get_u64(const uint8_t *buf) {
    return ((uint64_t)get_u32(buf) << 32) | (uint64_t)get_u32(buf + 4);
}

/* Get string from packet at offset, advance offset. Returns pointer into buf */
static const char *get_string(const uint8_t *buf, size_t buf_len, size_t *off, uint32_t *slen) {
    if (*off + 4 > buf_len) return NULL;
    uint32_t len = get_u32(buf + *off);
    *off += 4;
    if (*off + len > buf_len) return NULL;
    if (slen) *slen = len;
    const char *s = (const char *)(buf + *off);
    *off += len;
    return s;
}

/* Get uint32 from packet at offset */
static int get_uint32(const uint8_t *buf, size_t buf_len, size_t *off, uint32_t *val) {
    if (*off + 4 > buf_len) return -1;
    *val = get_u32(buf + *off);
    *off += 4;
    return 0;
}

/* Get uint64 from packet at offset */
static int get_uint64(const uint8_t *buf, size_t buf_len, size_t *off, uint64_t *val) {
    if (*off + 8 > buf_len) return -1;
    *val = get_u64(buf + *off);
    *off += 8;
    return 0;
}

/* ---- Output packet construction ---- */

static size_t out_off;

static void out_begin(uint8_t type) {
    out_off = 4; /* reserve space for length */
    out_buf[out_off++] = type;
}

static void out_u32(uint32_t val) {
    put_u32(out_buf + out_off, val);
    out_off += 4;
}

static void out_u64(uint64_t val) {
    put_u64(out_buf + out_off, val);
    out_off += 8;
}

static void out_string(const char *s, uint32_t len) {
    put_u32(out_buf + out_off, len);
    out_off += 4;
    if (len > 0) {
        memcpy(out_buf + out_off, s, len);
        out_off += len;
    }
}

static void out_cstring(const char *s) {
    out_string(s, (uint32_t)strlen(s));
}

static void out_data(const uint8_t *data, uint32_t len) {
    put_u32(out_buf + out_off, len);
    out_off += 4;
    if (len > 0) {
        memcpy(out_buf + out_off, data, len);
        out_off += len;
    }
}

static int out_send(void) {
    uint32_t pkt_len = (uint32_t)(out_off - 4);
    put_u32(out_buf, pkt_len);
    return write_all(out_buf, out_off);
}

/* ---- Attribute encoding ---- */

static void encode_attrs(const struct stat *st) {
    uint32_t flags = SSH_FILEXFER_ATTR_SIZE |
                     SSH_FILEXFER_ATTR_UIDGID |
                     SSH_FILEXFER_ATTR_PERMISSIONS |
                     SSH_FILEXFER_ATTR_ACMODTIME;
    out_u32(flags);
    out_u64((uint64_t)st->st_size);
    out_u32((uint32_t)st->st_uid);
    out_u32((uint32_t)st->st_gid);

    /* Convert EwokOS mode to standard POSIX mode bits */
    uint32_t mode = (uint32_t)st->st_mode;
    /* Add file type bits */
    if ((st->st_mode & 0040000) != 0)  /* directory check via raw mode */
        mode |= 0040000;
    out_u32(mode);
    out_u32((uint32_t)st->st_atime);
    out_u32((uint32_t)st->st_mtime);
}

static void encode_attrs_empty(void) {
    out_u32(0); /* no attributes */
}

/* Parse ATTRS from packet (skip over them) */
static int skip_attrs(const uint8_t *buf, size_t buf_len, size_t *off) {
    uint32_t flags;
    if (get_uint32(buf, buf_len, off, &flags) < 0) return -1;
    if (flags & SSH_FILEXFER_ATTR_SIZE) {
        uint64_t dummy;
        if (get_uint64(buf, buf_len, off, &dummy) < 0) return -1;
    }
    if (flags & SSH_FILEXFER_ATTR_UIDGID) {
        uint32_t dummy;
        if (get_uint32(buf, buf_len, off, &dummy) < 0) return -1;
        if (get_uint32(buf, buf_len, off, &dummy) < 0) return -1;
    }
    if (flags & SSH_FILEXFER_ATTR_PERMISSIONS) {
        uint32_t dummy;
        if (get_uint32(buf, buf_len, off, &dummy) < 0) return -1;
    }
    if (flags & SSH_FILEXFER_ATTR_ACMODTIME) {
        uint32_t dummy;
        if (get_uint32(buf, buf_len, off, &dummy) < 0) return -1;
        if (get_uint32(buf, buf_len, off, &dummy) < 0) return -1;
    }
    /* Extended attributes - skip pairs */
    if (flags & 0x80000000) {
        uint32_t count;
        if (get_uint32(buf, buf_len, off, &count) < 0) return -1;
        for (uint32_t i = 0; i < count; i++) {
            uint32_t slen;
            if (!get_string(buf, buf_len, off, &slen)) return -1;
            if (!get_string(buf, buf_len, off, &slen)) return -1;
        }
    }
    return 0;
}

/* Parse ATTRS and extract permissions */
static uint32_t parse_attrs_mode(const uint8_t *buf, size_t buf_len, size_t *off) {
    uint32_t flags;
    uint32_t mode = 0755; /* default */
    if (get_uint32(buf, buf_len, off, &flags) < 0) return mode;
    if (flags & SSH_FILEXFER_ATTR_SIZE) {
        uint64_t dummy;
        get_uint64(buf, buf_len, off, &dummy);
    }
    if (flags & SSH_FILEXFER_ATTR_UIDGID) {
        uint32_t dummy;
        get_uint32(buf, buf_len, off, &dummy);
        get_uint32(buf, buf_len, off, &dummy);
    }
    if (flags & SSH_FILEXFER_ATTR_PERMISSIONS) {
        get_uint32(buf, buf_len, off, &mode);
    }
    return mode;
}

/* ---- Handle management ---- */

static int alloc_handle_file(int fd, const char *path) {
    for (int i = 0; i < MAX_HANDLES; i++) {
        if (handles[i].type == 0) {
            handles[i].type = HANDLE_FILE;
            handles[i].fd = fd;
            handles[i].dirp = NULL;
            strncpy(handles[i].path, path ? path : "", sizeof(handles[i].path) - 1);
            handles[i].path[sizeof(handles[i].path) - 1] = '\0';
            return i;
        }
    }
    return -1;
}

static int alloc_handle_dir(DIR *dirp, const char *path) {
    for (int i = 0; i < MAX_HANDLES; i++) {
        if (handles[i].type == 0) {
            handles[i].type = HANDLE_DIR;
            handles[i].fd = -1;
            handles[i].dirp = dirp;
            strncpy(handles[i].path, path ? path : "", sizeof(handles[i].path) - 1);
            handles[i].path[sizeof(handles[i].path) - 1] = '\0';
            return i;
        }
    }
    return -1;
}

static void free_handle(int idx) {
    if (idx < 0 || idx >= MAX_HANDLES) return;
    if (handles[idx].type == HANDLE_FILE && handles[idx].fd >= 0)
        close(handles[idx].fd);
    if (handles[idx].type == HANDLE_DIR && handles[idx].dirp)
        closedir(handles[idx].dirp);
    handles[idx].type = 0;
    handles[idx].fd = -1;
    handles[idx].dirp = NULL;
}

static int get_handle_from_pkt(const uint8_t *buf, size_t buf_len, size_t *off) {
    uint32_t hlen;
    const char *hstr = get_string(buf, buf_len, off, &hlen);
    if (!hstr || hlen != 4) return -1;
    uint32_t idx = get_u32((const uint8_t *)hstr);
    if (idx >= MAX_HANDLES) return -1;
    if (handles[idx].type == 0) return -1;
    return (int)idx;
}

/* ---- Response helpers ---- */

static int send_status(uint32_t id, uint32_t code, const char *msg) {
    out_begin(SSH_FXP_STATUS);
    out_u32(id);
    out_u32(code);
    out_cstring(msg ? msg : "");
    out_cstring(""); /* language tag */
    return out_send();
}

static int send_handle(uint32_t id, int handle_idx) {
    uint8_t hbuf[4];
    put_u32(hbuf, (uint32_t)handle_idx);
    out_begin(SSH_FXP_HANDLE);
    out_u32(id);
    out_data(hbuf, 4);
    return out_send();
}

/* ---- Permission mapping from errno ---- */

static uint32_t errno_to_status(void) {
    switch (errno) {
    case ENOENT: return SSH_FX_NO_SUCH_FILE;
    case EACCES: case EPERM: return SSH_FX_PERMISSION_DENIED;
    default: return SSH_FX_FAILURE;
    }
}

/* ---- Packet handlers ---- */

static int handle_open(const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id, pflags;
    uint32_t slen;

    if (get_uint32(buf, len, &off, &id) < 0) return -1;
    const char *path = get_string(buf, len, &off, &slen);
    if (!path) return send_status(id, SSH_FX_BAD_MESSAGE, "bad open");
    if (get_uint32(buf, len, &off, &pflags) < 0)
        return send_status(id, SSH_FX_BAD_MESSAGE, "bad open flags");

    uint32_t mode = parse_attrs_mode(buf, len, &off);

    /* Convert SFTP flags to POSIX flags */
    int flags = 0;
    if ((pflags & SSH_FXF_READ) && (pflags & SSH_FXF_WRITE))
        flags = O_RDWR;
    else if (pflags & SSH_FXF_WRITE)
        flags = O_WRONLY;
    else
        flags = O_RDONLY;

    if (pflags & SSH_FXF_CREAT) flags |= O_CREAT;
    if (pflags & SSH_FXF_TRUNC) flags |= O_TRUNC;
    if (pflags & SSH_FXF_APPEND) flags |= O_APPEND;

    /* Need null-terminated path */
    char pathbuf[512];
    if (slen >= sizeof(pathbuf)) return send_status(id, SSH_FX_FAILURE, "path too long");
    memcpy(pathbuf, path, slen);
    pathbuf[slen] = '\0';

    int fd = open(pathbuf, flags);
    if (fd < 0)
        return send_status(id, errno_to_status(), strerror(errno));

    if (pflags & SSH_FXF_CREAT)
        fchmod(fd, mode & 0777);

    int h = alloc_handle_file(fd, pathbuf);
    if (h < 0) {
        close(fd);
        return send_status(id, SSH_FX_FAILURE, "too many handles");
    }
    return send_handle(id, h);
}

static int handle_close(const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id;

    if (get_uint32(buf, len, &off, &id) < 0) return -1;
    int h = get_handle_from_pkt(buf, len, &off);
    if (h < 0)
        return send_status(id, SSH_FX_FAILURE, "invalid handle");
    free_handle(h);
    return send_status(id, SSH_FX_OK, "");
}

static int handle_read(const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id, rlen;
    uint64_t roff;

    if (get_uint32(buf, len, &off, &id) < 0) return -1;
    int h = get_handle_from_pkt(buf, len, &off);
    if (h < 0) return send_status(id, SSH_FX_FAILURE, "invalid handle");
    if (get_uint64(buf, len, &off, &roff) < 0) return send_status(id, SSH_FX_BAD_MESSAGE, "");
    if (get_uint32(buf, len, &off, &rlen) < 0) return send_status(id, SSH_FX_BAD_MESSAGE, "");

    if (handles[h].type != HANDLE_FILE)
        return send_status(id, SSH_FX_FAILURE, "not a file handle");

    if (rlen > MAX_READ_REPLY_DATA) rlen = MAX_READ_REPLY_DATA;

    /* Seek to offset */
    if (lseek(handles[h].fd, (off_t)roff, SEEK_SET) < 0)
        return send_status(id, SSH_FX_FAILURE, "seek failed");

    uint8_t *data = (uint8_t *)malloc(rlen);
    if (!data) return send_status(id, SSH_FX_FAILURE, "out of memory");

    ssize_t n = read(handles[h].fd, data, rlen);
    if (n < 0) {
        free(data);
        return send_status(id, SSH_FX_FAILURE, strerror(errno));
    }
    if (n == 0) {
        free(data);
        return send_status(id, SSH_FX_EOF, "");
    }

    out_begin(SSH_FXP_DATA);
    out_u32(id);
    out_data(data, (uint32_t)n);
    free(data);
    if (out_send() < 0)
        return -1;
    return 0;
}

static int handle_write(const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id, dlen;
    uint64_t woff;

    if (get_uint32(buf, len, &off, &id) < 0) return -1;
    int h = get_handle_from_pkt(buf, len, &off);
    if (h < 0) return send_status(id, SSH_FX_FAILURE, "invalid handle");
    if (get_uint64(buf, len, &off, &woff) < 0) return send_status(id, SSH_FX_BAD_MESSAGE, "");

    const char *data = get_string(buf, len, &off, &dlen);
    if (!data) return send_status(id, SSH_FX_BAD_MESSAGE, "");

    if (handles[h].type != HANDLE_FILE)
        return send_status(id, SSH_FX_FAILURE, "not a file handle");

    if (lseek(handles[h].fd, (off_t)woff, SEEK_SET) < 0)
        return send_status(id, SSH_FX_FAILURE, "seek failed");

    size_t total = 0;
    while (total < dlen) {
        ssize_t w = write(handles[h].fd, data + total, dlen - total);
        if (w < 0) {
            if (errno == EINTR || errno == EAGAIN) continue;
            return send_status(id, SSH_FX_FAILURE, strerror(errno));
        }
        total += (size_t)w;
    }

    return send_status(id, SSH_FX_OK, "");
}

static int handle_stat_common(const uint8_t *buf, size_t len, int follow_link) {
    size_t off = 0;
    uint32_t id, slen;
    (void)follow_link; /* EwokOS doesn't distinguish lstat/stat */

    if (get_uint32(buf, len, &off, &id) < 0) return -1;
    const char *path = get_string(buf, len, &off, &slen);
    if (!path) return send_status(id, SSH_FX_BAD_MESSAGE, "");

    char pathbuf[512];
    if (slen >= sizeof(pathbuf)) return send_status(id, SSH_FX_FAILURE, "path too long");
    memcpy(pathbuf, path, slen);
    pathbuf[slen] = '\0';

    struct stat st;
    if (stat(pathbuf, &st) < 0)
        return send_status(id, errno_to_status(), strerror(errno));

    out_begin(SSH_FXP_ATTRS);
    out_u32(id);
    encode_attrs(&st);
    return out_send();
}

static int handle_fstat(const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id;

    if (get_uint32(buf, len, &off, &id) < 0) return -1;
    int h = get_handle_from_pkt(buf, len, &off);
    if (h < 0) return send_status(id, SSH_FX_FAILURE, "invalid handle");

    if (handles[h].type == HANDLE_DIR) {
        /* stat the directory path */
        struct stat st;
        if (stat(handles[h].path, &st) < 0)
            return send_status(id, errno_to_status(), strerror(errno));
        out_begin(SSH_FXP_ATTRS);
        out_u32(id);
        encode_attrs(&st);
        return out_send();
    }

    struct stat st;
    if (fstat(handles[h].fd, &st) < 0)
        return send_status(id, errno_to_status(), strerror(errno));

    out_begin(SSH_FXP_ATTRS);
    out_u32(id);
    encode_attrs(&st);
    return out_send();
}

static int handle_setstat(const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id, slen;

    if (get_uint32(buf, len, &off, &id) < 0) return -1;
    const char *path = get_string(buf, len, &off, &slen);
    if (!path) return send_status(id, SSH_FX_BAD_MESSAGE, "");

    char pathbuf[512];
    if (slen >= sizeof(pathbuf)) return send_status(id, SSH_FX_FAILURE, "path too long");
    memcpy(pathbuf, path, slen);
    pathbuf[slen] = '\0';

    uint32_t mode = parse_attrs_mode(buf, len, &off);
    chmod(pathbuf, mode & 0777);
    return send_status(id, SSH_FX_OK, "");
}

static int handle_fsetstat(const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id;

    if (get_uint32(buf, len, &off, &id) < 0) return -1;
    int h = get_handle_from_pkt(buf, len, &off);
    if (h < 0) return send_status(id, SSH_FX_FAILURE, "invalid handle");

    uint32_t mode = parse_attrs_mode(buf, len, &off);
    if (handles[h].type == HANDLE_FILE && handles[h].fd >= 0)
        fchmod(handles[h].fd, mode & 0777);
    return send_status(id, SSH_FX_OK, "");
}

static int handle_opendir(const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id, slen;

    if (get_uint32(buf, len, &off, &id) < 0) return -1;
    const char *path = get_string(buf, len, &off, &slen);
    if (!path) return send_status(id, SSH_FX_BAD_MESSAGE, "");

    char pathbuf[512];
    if (slen >= sizeof(pathbuf)) return send_status(id, SSH_FX_FAILURE, "path too long");
    memcpy(pathbuf, path, slen);
    pathbuf[slen] = '\0';

    DIR *dirp = opendir(pathbuf);
    if (!dirp)
        return send_status(id, errno_to_status(), strerror(errno));

    int h = alloc_handle_dir(dirp, pathbuf);
    if (h < 0) {
        closedir(dirp);
        return send_status(id, SSH_FX_FAILURE, "too many handles");
    }
    return send_handle(id, h);
}

static int handle_readdir(const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id;

    if (get_uint32(buf, len, &off, &id) < 0) return -1;
    int h = get_handle_from_pkt(buf, len, &off);
    if (h < 0) return send_status(id, SSH_FX_FAILURE, "invalid handle");
    if (handles[h].type != HANDLE_DIR || !handles[h].dirp)
        return send_status(id, SSH_FX_FAILURE, "not a directory handle");

    /* Read up to 32 entries at a time */
    #define MAX_DIR_ENTRIES 32
    struct {
        char name[256];
        struct stat st;
        int valid;
    } entries[MAX_DIR_ENTRIES];

    int count = 0;
    struct dirent *ent;
    while (count < MAX_DIR_ENTRIES && (ent = readdir(handles[h].dirp)) != NULL) {
        strncpy(entries[count].name, ent->d_name, sizeof(entries[count].name) - 1);
        entries[count].name[sizeof(entries[count].name) - 1] = '\0';

        /* Get full path for stat */
        char fullpath[512];
        size_t dlen2 = strlen(handles[h].path);
        if (dlen2 > 0 && handles[h].path[dlen2 - 1] == '/')
            snprintf(fullpath, sizeof(fullpath), "%s%s", handles[h].path, ent->d_name);
        else
            snprintf(fullpath, sizeof(fullpath), "%s/%s", handles[h].path, ent->d_name);

        if (stat(fullpath, &entries[count].st) == 0)
            entries[count].valid = 1;
        else {
            memset(&entries[count].st, 0, sizeof(struct stat));
            entries[count].valid = 0;
        }
        count++;
    }

    if (count == 0)
        return send_status(id, SSH_FX_EOF, "");

    out_begin(SSH_FXP_NAME);
    out_u32(id);
    out_u32((uint32_t)count);
    for (int i = 0; i < count; i++) {
        out_cstring(entries[i].name);
        /* Long name (ls -l style) */
        char longname[512];
        if (entries[i].valid) {
            uint32_t m = (uint32_t)entries[i].st.st_mode;
            char type_c = '-';
            if (m & 0040000) type_c = 'd';
            snprintf(longname, sizeof(longname), "%c%c%c%c%c%c%c%c%c%c %u %u %u %lu %s",
                     type_c,
                     (m & 0400) ? 'r' : '-', (m & 0200) ? 'w' : '-', (m & 0100) ? 'x' : '-',
                     (m & 040) ? 'r' : '-', (m & 020) ? 'w' : '-', (m & 010) ? 'x' : '-',
                     (m & 04) ? 'r' : '-', (m & 02) ? 'w' : '-', (m & 01) ? 'x' : '-',
                     (unsigned)entries[i].st.st_uid,
                     (unsigned)entries[i].st.st_gid,
                     (unsigned)entries[i].st.st_size,
                     (unsigned long)entries[i].st.st_mtime,
                     entries[i].name);
        } else {
            snprintf(longname, sizeof(longname), "---------- 0 0 0 0 %s", entries[i].name);
        }
        out_cstring(longname);
        if (entries[i].valid)
            encode_attrs(&entries[i].st);
        else
            encode_attrs_empty();
    }
    return out_send();
}

static int handle_remove(const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id, slen;

    if (get_uint32(buf, len, &off, &id) < 0) return -1;
    const char *path = get_string(buf, len, &off, &slen);
    if (!path) return send_status(id, SSH_FX_BAD_MESSAGE, "");

    char pathbuf[512];
    if (slen >= sizeof(pathbuf)) return send_status(id, SSH_FX_FAILURE, "path too long");
    memcpy(pathbuf, path, slen);
    pathbuf[slen] = '\0';

    if (unlink(pathbuf) < 0)
        return send_status(id, errno_to_status(), strerror(errno));
    return send_status(id, SSH_FX_OK, "");
}

static int handle_mkdir(const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id, slen;

    if (get_uint32(buf, len, &off, &id) < 0) return -1;
    const char *path = get_string(buf, len, &off, &slen);
    if (!path) return send_status(id, SSH_FX_BAD_MESSAGE, "");

    char pathbuf[512];
    if (slen >= sizeof(pathbuf)) return send_status(id, SSH_FX_FAILURE, "path too long");
    memcpy(pathbuf, path, slen);
    pathbuf[slen] = '\0';

    uint32_t mode = parse_attrs_mode(buf, len, &off);
    if (mkdir(pathbuf, mode & 0777) < 0)
        return send_status(id, errno_to_status(), strerror(errno));
    return send_status(id, SSH_FX_OK, "");
}

static int handle_rmdir(const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id, slen;

    if (get_uint32(buf, len, &off, &id) < 0) return -1;
    const char *path = get_string(buf, len, &off, &slen);
    if (!path) return send_status(id, SSH_FX_BAD_MESSAGE, "");

    char pathbuf[512];
    if (slen >= sizeof(pathbuf)) return send_status(id, SSH_FX_FAILURE, "path too long");
    memcpy(pathbuf, path, slen);
    pathbuf[slen] = '\0';

    if (rmdir(pathbuf) < 0)
        return send_status(id, errno_to_status(), strerror(errno));
    return send_status(id, SSH_FX_OK, "");
}

static int handle_realpath(const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id, slen;

    if (get_uint32(buf, len, &off, &id) < 0) return -1;
    const char *path = get_string(buf, len, &off, &slen);
    if (!path) return send_status(id, SSH_FX_BAD_MESSAGE, "");

    char pathbuf[512];
    if (slen >= sizeof(pathbuf)) return send_status(id, SSH_FX_FAILURE, "path too long");
    memcpy(pathbuf, path, slen);
    pathbuf[slen] = '\0';

    /* Resolve path - simple implementation */
    char resolved[512];
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

    /* Remove trailing slash (except root) */
    size_t rlen = strlen(resolved);
    while (rlen > 1 && resolved[rlen - 1] == '/') {
        resolved[--rlen] = '\0';
    }

    out_begin(SSH_FXP_NAME);
    out_u32(id);
    out_u32(1); /* count */
    out_cstring(resolved);
    out_cstring(resolved); /* long name */
    encode_attrs_empty();
    return out_send();
}

static int handle_rename(const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t id, slen1, slen2;

    if (get_uint32(buf, len, &off, &id) < 0) return -1;
    const char *oldpath = get_string(buf, len, &off, &slen1);
    if (!oldpath) return send_status(id, SSH_FX_BAD_MESSAGE, "");
    const char *newpath = get_string(buf, len, &off, &slen2);
    if (!newpath) return send_status(id, SSH_FX_BAD_MESSAGE, "");

    char old_buf[512], new_buf[512];
    if (slen1 >= sizeof(old_buf) || slen2 >= sizeof(new_buf))
        return send_status(id, SSH_FX_FAILURE, "path too long");
    memcpy(old_buf, oldpath, slen1); old_buf[slen1] = '\0';
    memcpy(new_buf, newpath, slen2); new_buf[slen2] = '\0';

    if (rename(old_buf, new_buf) < 0)
        return send_status(id, errno_to_status(), strerror(errno));
    return send_status(id, SSH_FX_OK, "");
}

/* ---- Main packet dispatch ---- */

static int process_packet(uint8_t type, const uint8_t *payload, size_t payload_len) {
    switch (type) {
    case SSH_FXP_OPEN:
        return handle_open(payload, payload_len);
    case SSH_FXP_CLOSE:
        return handle_close(payload, payload_len);
    case SSH_FXP_READ:
        return handle_read(payload, payload_len);
    case SSH_FXP_WRITE:
        return handle_write(payload, payload_len);
    case SSH_FXP_LSTAT:
        return handle_stat_common(payload, payload_len, 0);
    case SSH_FXP_STAT:
        return handle_stat_common(payload, payload_len, 1);
    case SSH_FXP_FSTAT:
        return handle_fstat(payload, payload_len);
    case SSH_FXP_SETSTAT:
        return handle_setstat(payload, payload_len);
    case SSH_FXP_FSETSTAT:
        return handle_fsetstat(payload, payload_len);
    case SSH_FXP_OPENDIR:
        return handle_opendir(payload, payload_len);
    case SSH_FXP_READDIR:
        return handle_readdir(payload, payload_len);
    case SSH_FXP_REMOVE:
        return handle_remove(payload, payload_len);
    case SSH_FXP_MKDIR:
        return handle_mkdir(payload, payload_len);
    case SSH_FXP_RMDIR:
        return handle_rmdir(payload, payload_len);
    case SSH_FXP_REALPATH:
        return handle_realpath(payload, payload_len);
    case SSH_FXP_RENAME:
        return handle_rename(payload, payload_len);
    case SSH_FXP_READLINK:
    case SSH_FXP_SYMLINK: {
        /* Unsupported - extract id and return failure */
        uint32_t id = 0;
        if (payload_len >= 4) id = get_u32(payload);
        return send_status(id, SSH_FX_OP_UNSUPPORTED, "not supported");
    }
    default: {
        uint32_t id = 0;
        if (payload_len >= 4) id = get_u32(payload);
        return send_status(id, SSH_FX_OP_UNSUPPORTED, "unknown request");
    }
    }
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    memset(handles, 0, sizeof(handles));

    /* Main packet loop */
    while (1) {
        uint8_t lenbuf[4];
        if (read_exact(lenbuf, 4) < 0)
            break;

        uint32_t pkt_len = get_u32(lenbuf);
        if (pkt_len == 0 || pkt_len > MAX_PKT_SIZE)
            break;

        if (read_exact(pkt_buf, pkt_len) < 0)
            break;

        uint8_t type = pkt_buf[0];
        const uint8_t *payload = pkt_buf + 1;
        size_t payload_len = pkt_len - 1;

        if (type == SSH_FXP_INIT) {
            /* Handle INIT specially - has version but no request id */
            uint32_t client_ver = 0;
            if (payload_len >= 4)
                client_ver = get_u32(payload);
            (void)client_ver;

            /* Send VERSION response */
            out_begin(SSH_FXP_VERSION);
            out_u32(SFTP_VERSION);
            if (out_send() < 0)
                break;
            continue;
        }

        if (process_packet(type, payload, payload_len) < 0)
            break;
    }

    /* Cleanup open handles */
    for (int i = 0; i < MAX_HANDLES; i++)
        free_handle(i);

    return 0;
}
