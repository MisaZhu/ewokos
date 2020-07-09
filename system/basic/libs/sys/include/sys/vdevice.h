#ifndef VDEVICE_H
#define VDEVICE_H

#include <sys/fsinfo.h>
#include <sys/proto.h>

#ifdef __cplusplus
extern "C" {
#endif


#define MAX_TRUST_PID 10

typedef struct {
	bool terminated;
	bool single_task;
	char name[FS_NODE_NAME_MAX];
	void* extra_data;
	int (*dev_cntl)(int from_pid, int cmd, proto_t* in, proto_t* ret, void* p);
	int (*open)(int fd, int from_pid, fsinfo_t* info, int oflag, void* p);
	int (*create)(fsinfo_t* info_to, fsinfo_t* info, void* p);
	int (*close)(int fd, int from_pid, fsinfo_t* info, void* p);
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
	void (*interrupt)(proto_t* in, void* p);
	int (*loop_step)(void* p);
} vdevice_t;

extern int device_run(vdevice_t* dev, const char* mnt_point, int mnt_type);

extern int dev_cntl(const char* fname, int cmd, proto_t* in, proto_t* out);
extern int dev_cntl_by_pid(int pid, int cmd, proto_t* in, proto_t* out);
extern int dev_get_pid(const char* fname);

#ifdef __cplusplus
}
#endif

#endif
