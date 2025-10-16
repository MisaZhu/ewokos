#ifndef __ARCH_VIRTFS_H__
#define __ARCH_VIRTFS_H__
#include <stdint.h>

typedef void* virtfs_t;

typedef struct virtfs_stat{
    int16_t size;
    int16_t type;
    int32_t dev;
    uint8_t pad[13];
    int32_t mode;
    int32_t atime;
    int32_t mtime;
    int64_t length;
}__attribute__((packed)) virtfs_stat_t;

#define VIRTFS_TYPE_DIR 0x04
#define VIRTFS_TYPE_FILE 0x08

struct virtfs_dir_entry
{
    uint8_t qid_type;
    uint32_t qid_vers;
    uint64_t qid_path;
    uint64_t offset;
    uint8_t type;
    uint16_t nlen;
    char name[0];
} __attribute__((packed));

virtfs_t virtfs_init(void);
int virtfs_set_version(virtfs_t fs, const char *version);
int virtfs_attach(virtfs_t fs, uint16_t tag, uint32_t fid, uint32_t afid, const char *uname, const char *aname);
int virtfs_readdir(virtfs_t fs, uint16_t tag, uint32_t fid, void *buf, uint64_t offset, uint32_t max);
int virtfs_walk(virtfs_t fs, uint16_t tag, uint32_t fid, uint32_t newfid, const char *wname);
int virtfs_open(virtfs_t fs, uint16_t tag, uint32_t fid, uint32_t mode);    
int virtfs_read(virtfs_t fs, uint16_t tag, uint32_t fid, void *buf, uint64_t offset, uint32_t max);
int virtfs_close(virtfs_t fs, uint16_t tag, uint32_t fid);
#endif