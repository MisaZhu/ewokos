#include <arch/virt/virtio.h>

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#define EIO 5

#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define VIRTFS_RLERROR 7
#define VIRTFS_TREADDIR 40
#define VIRTFS_TFSYNC 50
#define VIRTFS_TVERSION 100
#define VIRTFS_TAUTH 102
#define VIRTFS_TATTACH 104
#define VIRTFS_TERROR 106
#define VIRTFS_TFLUSH 108
#define VIRTFS_TWALK 110
#define VIRTFS_TOPEN 112
#define VIRTFS_TCREATE 114
#define VIRTFS_TREAD 116
#define VIRTFS_TWRITE 118
#define VIRTFS_TCLUNK 120
#define VIRTFS_TREMOVE 122
#define VIRTFS_TSTAT 124
#define VIRTFS_TWSTAT 126


#define HAL_DEBUG 0
#if HAL_DEBUG
#define DBG(fmt, ...) klog(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...) \
	do                   \
	{                    \
	} while (0)
#endif


typedef struct
{
    virtio_dev_t virtio;
} *virtfs_t;

typedef struct virtfs_stat
{
    int16_t size;
    int16_t type;
    int32_t dev;
    uint8_t pad[13];
    int32_t mode;
    int32_t atime;
    int32_t mtime;
    int64_t length;
} __attribute__((packed)) virtfs_stat_t;

struct virtfs_header
{
    uint32_t size;
    uint8_t type;
    uint16_t tag;
} __attribute__((packed));

static uint32_t virtrfs_parse_respon(int req, void* data, int size)
{
    if(size < sizeof(struct virtfs_header))
        return -1;
    struct virtfs_header *hdr = (struct virtfs_header *)data;
    if(hdr->type != req + 1){
        uint32_t errno = *(uint32_t*)(data + sizeof(struct virtfs_header));
        DBG("virtfs reqest %d error! tag:%d size:%d type:%d errno:%d\n", req, hdr->tag, hdr->size, hdr->type, errno);
        if(hdr->type == VIRTFS_RLERROR)
            return errno;  
        else
            return EIO; 
    }
    return 0;
}

static uint32_t virtfs_vfill(void *buf, const char *fmt, va_list ap)
{
    int i;
    uint32_t copied = 0;

    for (i = 0; fmt[i]; i++)
    {
        switch (fmt[i])
        {
        case 'b':
        {
            uint8_t val = va_arg(ap, int);
            *((uint8_t *)(buf + copied)) = val;
            copied += sizeof(val);
            break;
        }
        case 'w':
        {
            uint16_t val = va_arg(ap, int);
            *((uint16_t *)(buf + copied)) = val;
            copied += sizeof(val);
            break;
        }
        case 'd':
        {
            uint32_t val = va_arg(ap, uint32_t);
            *((uint32_t *)(buf + copied)) = val;
            copied += sizeof(val);
            break;
        }
        case 'q':
        {
            uint64_t val = va_arg(ap, uint64_t);
            *((uint64_t *)(buf + copied)) = val;
            copied += sizeof(val);
            break;
        }
        case 's':
        {
            const char *str = va_arg(ap, const char *);
            *((uint16_t *)(buf + copied)) = strlen(str);
            memcpy((uint8_t *)(buf + copied + sizeof(uint16_t)), str, strlen(str));
            copied += sizeof(uint16_t) + strlen(str);
            break;
        }
        default:
            break;
        }
    }
    return copied;
}

uint32_t virtfs_fill(void *buf, const char *fmt, ...)
{
    uint32_t ret;
    va_list ap;

    va_start(ap, fmt);
    ret = virtfs_vfill(buf, fmt, ap);
    va_end(ap);

    return ret;
}

virtfs_t virtfs_init(void)
{
    virtio_dev_t virtio = virtio_get(VIRTIO_ID_9P);
    if (virtio == NULL)
    {
        //HAL_DBG("virtio_get VIRTIO_ID_9P failed!\n");
        return NULL;
    }
    virtio_init(virtio, 0);
    virtfs_t fs = (virtfs_t)malloc(sizeof(*fs));
    if (fs == NULL)
    {
        return NULL;
    }
    fs->virtio = virtio;

    return fs;
}

int virtfs_set_version(virtfs_t fs, const char *version, uint32_t msize)
{

    uint32_t len = strlen(version) + 14;
    uint8_t buf[len];

    struct virtfs_header *hdr = (struct virtfs_header *)buf;
    hdr->type = VIRTFS_TVERSION;
    hdr->tag = 1;
    hdr->size = sizeof(struct virtfs_header) + virtfs_fill(buf + sizeof(struct virtfs_header), "ds", msize, version);

    uint8_t resp[32];
    int ret = virtio_send_request(fs->virtio, buf, len, resp, sizeof(resp));
    return virtrfs_parse_respon(VIRTFS_TVERSION, resp, ret);
}

int virtfs_attach(virtfs_t fs, uint16_t tag, uint32_t fid, uint32_t afid, const char *uname, const char *aname)
{
    uint8_t req[256];
    struct virtfs_header *hdr = (struct virtfs_header *)req;
    hdr->type = VIRTFS_TATTACH;
    hdr->tag = tag;
    hdr->size = sizeof(struct virtfs_header) + virtfs_fill(req + sizeof(struct virtfs_header), "ddssd", fid, afid, uname, aname, 0);

    uint8_t resp[32];
    int ret = virtio_send_request(fs->virtio, req, hdr->size, &resp, sizeof(resp));
    return virtrfs_parse_respon(VIRTFS_TATTACH, resp, ret);
}

int virtfs_walk(virtfs_t fs, uint16_t tag, uint32_t fid, uint32_t newfid, const char *wname)
{
    uint8_t req[256];
    struct virtfs_header *hdr = (struct virtfs_header *)req;
    hdr->type = VIRTFS_TWALK;
    hdr->tag = tag;
    hdr->size = sizeof(struct virtfs_header) + virtfs_fill(req + sizeof(struct virtfs_header), "ddws", fid, newfid, 1, wname);

    uint8_t resp[32];
    int ret = virtio_send_request(fs->virtio, req, hdr->size, &resp, sizeof(resp));
    return virtrfs_parse_respon(VIRTFS_TWALK, resp, ret);
}

int virtfs_open(virtfs_t fs, uint16_t tag, uint32_t fid, uint32_t mode)
{
    uint8_t req[256];
    struct virtfs_header *hdr = (struct virtfs_header *)req;
    hdr->type = VIRTFS_TOPEN;
    hdr->tag = tag;
    hdr->size = sizeof(struct virtfs_header) + virtfs_fill(req + sizeof(struct virtfs_header), "dd", fid, mode);

    uint8_t resp[32];
    int ret = virtio_send_request(fs->virtio, req, hdr->size, &resp, sizeof(resp));
    return virtrfs_parse_respon(VIRTFS_TOPEN, resp, ret);
}

int virtfs_read(virtfs_t fs, uint16_t tag, uint32_t fid, void *buf, uint64_t offset, uint32_t size)
{
    uint8_t req[256];
    uint8_t resp[1036];
    struct virtfs_header *hdr = (struct virtfs_header *)req;
    int total = 0;
    int readlen = 0;
    do{
        uint32_t chunk = MIN(size, 1024);
        hdr->type = VIRTFS_TREAD;
        hdr->tag = tag;
        hdr->size = sizeof(struct virtfs_header) + virtfs_fill(req + sizeof(struct virtfs_header), "dqd", fid, offset, chunk);

        int ret = virtio_send_request(fs->virtio, req, hdr->size, &resp, sizeof(resp));
        hdr = (struct virtfs_header *)resp;
        if(virtrfs_parse_respon(VIRTFS_TREAD, resp, ret) < 0)
            return -EIO;
        readlen = *(uint32_t *)(resp + 7);

        memcpy(buf, resp + 11, readlen);
        buf += readlen;
        size -= readlen;
        offset += readlen;
        total += readlen;
    }while(size > 0 && readlen > 0);
    return total;
}

int virtfs_close(virtfs_t fs, uint16_t tag, uint32_t fid)
{
    uint8_t req[256];
    struct virtfs_header *hdr = (struct virtfs_header *)req;
    hdr->type = VIRTFS_TCLUNK;
    hdr->tag = tag;
    hdr->size = sizeof(struct virtfs_header) + virtfs_fill(req + sizeof(struct virtfs_header), "d", fid);

    uint8_t resp[32];
    int ret = virtio_send_request(fs->virtio, req, hdr->size, &resp, sizeof(resp));
    return virtrfs_parse_respon(VIRTFS_TCLUNK, resp, ret);
}

int virtfs_stat(virtfs_t fs, uint16_t tag, uint32_t fid, virtfs_stat_t *stat)
{
    uint8_t req[256];
    struct virtfs_header *hdr = (struct virtfs_header *)req;
    hdr->type = VIRTFS_TSTAT;
    hdr->tag = tag;
    hdr->size = sizeof(struct virtfs_header) + virtfs_fill(req + sizeof(struct virtfs_header), "d", fid);

    uint8_t resp[256];
    int ret = virtio_send_request(fs->virtio, req, hdr->size, &resp, sizeof(resp));
    if(virtrfs_parse_respon(VIRTFS_TSTAT, resp, ret) == 0){
        hdr = (struct virtfs_header *)resp;
        memcpy(stat, resp + 9, sizeof(virtfs_stat_t));
        return 0;
    }
    return -EIO;
}

int virtfs_readdir(virtfs_t fs, uint16_t tag, uint32_t fid, void *buf, uint64_t offset, uint32_t max)
{
    uint8_t req[256];
    struct virtfs_header *hdr = (struct virtfs_header *)req;
    hdr->type = VIRTFS_TREADDIR;
    hdr->tag = tag;
    hdr->size = sizeof(struct virtfs_header) + virtfs_fill(req + sizeof(struct virtfs_header), "dqd", fid, offset, max);

    uint8_t resp[1036];
    int ret = virtio_send_request(fs->virtio, req, hdr->size, &resp, sizeof(resp));
    if(virtrfs_parse_respon(VIRTFS_TREADDIR, resp, ret) == 0){
        uint32_t len = MIN(*(uint32_t*)&resp[7], max);
        memcpy(buf, resp + 11, len);
        return len;
    }
    return -EIO;
}

int virtfs_sync(virtfs_t fs, uint16_t tag, uint32_t fid)
{
    uint8_t req[256];
    struct virtfs_header *hdr = (struct virtfs_header *)req;
    hdr->type = VIRTFS_TFSYNC;
    hdr->tag = tag;
    hdr->size = sizeof(struct virtfs_header) + virtfs_fill(req + sizeof(struct virtfs_header), "dd", fid, 1);

    uint8_t resp[32];
    int ret = virtio_send_request(fs->virtio, req, hdr->size, &resp, sizeof(resp));
    return virtrfs_parse_respon(VIRTFS_TFSYNC, resp, ret);
}