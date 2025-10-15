#include <arch/virt/virtio.h>
#define P9_TVERSION 100
#define P9_TATTACH 104

typedef struct{
    virtio_dev_t virtio;
}*virtfs_t;

struct p9_header {
    uint32_t size;
    uint8_t type;
    uint16_t tag;
}__attribute__((packed));

struct p9_resp {
    uint32_t size;        // 消息总大小
    uint8_t type;         // 消息类型
    uint16_t tag;         // 消息标签
    uint32_t qid;         // QID标识
    uint32_t iounit;      // IO单元大小
    char data[256];       // 文件名
};

virtfs_t virtfs_init(void) {
	virtio_dev_t virtio = virtio_get(VIRTIO_ID_9P);
    if(virtio == NULL) {
        klog("virtio_get VIRTIO_ID_9P failed!\n");
        return NULL;
    }
    virtio_init(virtio, 0);
    virtfs_t fs= (virtfs_t)malloc(sizeof(*fs));
    if(fs == NULL) {
        return NULL;
    }
    fs->virtio = virtio;

    return fs;
}

int virtfs_set_version(virtfs_t fs, const char *version) {

    uint32_t len = strlen(version) + 14;
    uint8_t buf[len];
    
    struct p9_header *hdr = (struct p9_header *)buf;
    hdr->size = len;
    hdr->type = P9_TVERSION;
    hdr->tag = 1;
      uint32_t *msize_ptr = (uint32_t *)(buf + 7);
    *msize_ptr = 16384;
    
    uint16_t* size = (uint16_t *)(buf + 11);
    *size = strlen(version);
    strcpy((char *)(buf + 13), version);
    
    struct p9_resp resp;
    int ret = virtio_send_request(fs->virtio, buf, len, &resp, sizeof(resp));
    if(ret != sizeof(resp)) {
        return -1;
    }
    return resp.size;
}

int virtfs_attach(virtfs_t fs, uint16_t tag, uint32_t fid, uint32_t afid, const char *uname, const char *aname) {
    uint32_t uname_len = strlen(uname);
    uint32_t aname_len = strlen(aname);
    uint32_t len = 7 + 4 + 4 + 2 + uname_len + 2 + aname_len + 4;
    
    uint8_t buf[len];
    struct p9_header *hdr = (struct p9_header *)buf;
    hdr->size = len;
    hdr->type = P9_TATTACH;
    hdr->tag = tag;
    
    uint32_t *fid_ptr = (uint32_t *)(buf + 7);
    *fid_ptr = fid;
    
    uint32_t *afid_ptr = (uint32_t *)(buf + 11);
    *afid_ptr = afid;
    
    uint16_t *uname_len_ptr = (uint16_t *)(buf + 15);
    *uname_len_ptr = uname_len;
    
    memcpy(buf + 17, uname, uname_len);
    
    uint16_t *aname_len_ptr = (uint16_t *)(buf + 17 + uname_len);
    *aname_len_ptr = aname_len;
    
    memcpy(buf + 19 + uname_len, aname, aname_len);
    
    struct p9_resp resp;
    int ret = virtio_send_request(fs->virtio, buf, len, &resp, sizeof(resp));
    if(ret != sizeof(resp)) {
        return -1;
    }
    return resp.size;
}