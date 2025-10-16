#ifndef DEVICE_CMD_H
#define DEVICE_CMDH

#include <ewoksys/fsinfo.h>
#include <ewoksys/proto.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int dev_set(int dev_pid, fsinfo_t* info);
extern int dev_stat(int dev_pid, fsinfo_t* info, node_stat_t* stat);
extern int dev_create(int dev_pid, fsinfo_t* info_to, fsinfo_t* info);
extern int dev_unlink(int dev_pid, uint32_t node, const char* fname);
extern int dev_open(int dev_pid, int fd, fsinfo_t* node, int oflag);
extern int dev_read(int dev_pid, int fd, fsinfo_t* node, int32_t offset, void* buf, uint32_t size);
extern int dev_write(int dev_pid, int fd, fsinfo_t* node, int32_t offset, const void* buf, uint32_t size);
extern int dev_flush(int dev_pid, int fd, uint32_t node, int8_t wait);
extern int dev_dma(int dev_pid, int fd, uint32_t node, int* size);
extern int dev_read_block(int dev_pid, void* buf, uint32_t size, int32_t index);
extern int dev_write_block(int dev_pid, const void* buf, uint32_t size, int32_t index);
extern int dev_fcntl(int dev_pid, int fd, fsinfo_t* info, int cmd, proto_t* arg_in, proto_t* arg_out);


#ifdef __cplusplus
}
#endif

#endif
