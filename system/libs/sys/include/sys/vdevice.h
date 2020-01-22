#ifndef VDEVICE_H
#define VDEVICE_H

#include <fsinfo.h>
#include <proto.h>

typedef struct {
	char name[FS_NODE_NAME_MAX];
	int (*open)(int fd, int from_pid, fsinfo_t* info, int oflag, void* p);
	int (*create)(fsinfo_t* info_to, fsinfo_t* info, void* p);
	int (*close)(int fd, int from_pid, fsinfo_t* info, void* p);
	int (*closed)(int fd, int from_pid, fsinfo_t* info, void* p);
	int (*read)(int fd, int from_pid, fsinfo_t* info, void* buf, int size, int offset, void* p);
	int (*write)(int fd, int from_pid, fsinfo_t* info, const void* buf, int size, int offset, void* p);
	int (*read_block)(int from_pid, void* buf, int size, int index, void* p);
	int (*write_block)(int from_pid, const void* buf, int size, int index, void* p);
	int (*dma)(int fd, int from_pid, fsinfo_t* info, int* size, void* p);
	int (*flush)(int fd, int from_pid, fsinfo_t* info, void* p);
	int (*fcntl)(int fd, int from_pid, fsinfo_t* info, int cmd, proto_t* in, proto_t* out, void* p);
	int (*mount)(fsinfo_t* mnt_point, void* p);
	int (*umount)(fsinfo_t* mnt_point, void* p);
	int (*unlink)(fsinfo_t* info, const char *fname, void* p);
	int (*clear_buffer)(fsinfo_t* info, void* p);
	int (*loop_step)(void* p);
} vdevice_t;

extern int device_run(vdevice_t* dev, const char* mnt_point, int mnt_type, void* p, int block);

#endif
