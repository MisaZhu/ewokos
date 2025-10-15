#ifndef __ARCH_VIRTFS_H__
#define __ARCH_VIRTFS_H__

typedef void* virtfs_t;

virtfs_t virtfs_init(void);
int virtfs_set_version(virtfs_t fs, const char *version);
int virtfs_attach(virtfs_t fs, uint16_t tag, uint32_t fid, uint32_t afid, const char *uname, const char *aname);
 
#endif