#ifndef VFS_H
#define VFS_H

#include <ewoksys/fsinfo.h>
#include <ewoksys/mstr.h>
#include <ewoksys/proto.h>

#ifdef __cplusplus
extern "C" {
#endif


const char* vfs_fullname(const char* fname);

int       vfs_read_pipe(uint32_t node, void* buf, uint32_t size, bool block);
int       vfs_write_pipe(uint32_t node, const void* buf, uint32_t size, bool block);
int       vfs_open(fsinfo_t* info, int wr);
int       vfs_close(int fd);
int       vfs_new_node(fsinfo_t* info, uint32_t node_to);
int       vfs_del_node(uint32_t node);
int       vfs_access(const char* fname);
int       vfs_get_by_name(const char* fname, fsinfo_t* info);
int       vfs_get_by_node(uint32_t node, fsinfo_t* info);
int       vfs_get_by_fd(int fd, fsinfo_t* info);
int       vfs_tell(int fd);
int       vfs_seek(int fd, int offset);
int       vfs_set(fsinfo_t* info);
int       vfs_get_mount_by_id(int id, mount_t* mount);

fsinfo_t* vfs_kids(uint32_t node, uint32_t* num);

int       vfs_mount(uint32_t mount_node_to, uint32_t node);
int       vfs_umount(uint32_t node);

int       vfs_create(const char* fname, fsinfo_t* ret, int type, bool vfs_node_only, bool autodir);
void*     vfs_readfile(const char* fname, int* sz);
int       vfs_parse_name(const char* fname, str_t* dir, str_t* name);
int       vfs_dup(int fd);
int       vfs_dup2(int fd, int to);
int       vfs_open_pipe(int fd[2]);
int       vfs_flush(int fd, bool wait);
int32_t   vfs_dma(int fd, int* size); 

int       vfs_read_block(int pid, void* buf, uint32_t size, int32_t index);
int       vfs_write_block(int pid, const void* buf, uint32_t size, int32_t index);

int       vfs_read(int fd, fsinfo_t* info, void* buf, uint32_t size);
int       vfs_write(int fd, fsinfo_t* info, const void* buf, uint32_t size);

int       vfs_fcntl(int fd, int cmd, proto_t* in, proto_t* out);
int       vfs_fcntl_wait(int fd, int cmd, proto_t* in);

#ifdef __cplusplus
}
#endif

#endif
