#ifndef VFSCLIENT_H
#define VFSCLIENT_H

#include <ewoksys/fsinfo.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	VFS_GET_BY_NAME = 0,
	VFS_GET_BY_NODE,
	VFS_GET_BY_FD,
	VFS_SET_FSINFO,
	VFS_NEW_NODE,
	VFS_OPEN,
	VFS_PIPE_OPEN,
	VFS_PIPE_WRITE,
	VFS_PIPE_READ,
	VFS_DUP,
	VFS_DUP2,
	VFS_CLOSE,
	VFS_TELL,
	VFS_SEEK,
	VFS_DEL_NODE,
	VFS_MOUNT,
	VFS_UMOUNT,
	VFS_GET_MOUNT_BY_ID,
	VFS_GET_KIDS,
	VFS_GET_FLAGS,
	VFS_SET_FLAGS,
	VFS_PROC_CLONE,
	VFS_PROC_EXIT
};

enum {
	VFS_ACCESS_R = 0,
	VFS_ACCESS_W,
	VFS_ACCESS_X
};

int  vfs_check_access(int pid, fsinfo_t* info, int mode);

#ifdef __cplusplus
}
#endif

#endif
