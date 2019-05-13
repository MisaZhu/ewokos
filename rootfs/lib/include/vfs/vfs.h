#ifndef VFS_NODE_H
#define VFS_NODE_H

#include <fsinfo.h>
#include <tstr.h>
#include <vfs/vfscmd.h>
#define VFS_DIR_SIZE 0xffffffff

#define VFS_NAME "kserv.vfsd"

typedef struct {
	char dev_name[DEV_NAME_MAX];
	int32_t dev_index;
	int32_t dev_serv_pid;
	uint32_t node_old;
} mount_t;

uint32_t vfs_add(int32_t fd, const char* name, uint32_t size, void* data);

int32_t vfs_del(const char* fname);

tstr_t* vfs_full_name_by_node(uint32_t node);

tstr_t* vfs_short_name_by_node(uint32_t node);

int32_t vfs_open(const char* fname, int32_t flags);

int32_t vfs_close(int32_t fd);

int32_t vfs_node_by_addr(uint32_t node, fs_info_t* info);

int32_t vfs_node_by_name(const char* fname, fs_info_t* info);

int32_t vfs_node_by_fd(int32_t fd, fs_info_t* info);

int32_t vfs_node_update(fs_info_t* info);

int32_t vfs_pipe_open(int32_t fds[2]);

uint32_t vfs_mount(const char* fname, const char* devName, int32_t devIndex, bool isFile);

int32_t vfs_unmount(uint32_t node);

int32_t vfs_kid(uint32_t node, int32_t index, fs_info_t* kid);

int32_t vfs_mount_by_index(int32_t index, mount_t* mnt);

int32_t vfs_mount_by_fname(const char* fname, mount_t* mnt);

#endif
