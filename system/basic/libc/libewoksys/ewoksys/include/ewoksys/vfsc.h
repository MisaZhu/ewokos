#ifndef VFSCLIENT_H
#define VFSCLIENT_H

#include <ewoksys/fsinfo.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VFS_EVT_RD	    0x01
#define VFS_EVT_WR	    0x02
#define VFS_EVT_RW	    (VFS_EVT_RD | VFS_EVT_WR)
#define VFS_EVT_CLOSE	0x04
#define VFS_EVT_ERR 	0x08
#define VFS_EVT_NVAL 	0x10

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
	VFS_DEL_NODE,
	VFS_MOUNT,
	VFS_UMOUNT,
	VFS_GET_MOUNT_BY_ID,
	VFS_GET_KIDS,
	VFS_PROC_CLONE,
	VFS_PROC_EXIT,
	VFS_BLOCK,
	VFS_WAKEUP,
	VFS_GET_POLL_EVENTS,
	VFS_CLEAR_POLL_EVENTS
};

int  vfs_check_access(int pid, fsinfo_t* info, int mode);

typedef struct {
	uint32_t node;
	uint32_t event;
} vfs_poll_event_t;

#ifdef __cplusplus
}
#endif

#endif
