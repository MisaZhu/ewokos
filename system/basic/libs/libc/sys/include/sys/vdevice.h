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
	char name[FS_NODE_NAME_MAX];
	void* extra_data;
	int (*dev_cntl)(int from_pid, int cmd, proto_t* in, proto_t* ret, void* p);
	int (*open)(int fd, int from_pid, fsinfo_t* info, int oflag, void* p);
	int (*create)(uint32_t node_t, uint32_t node, void* p, fsinfo_t* info);
	int (*close)(int fd, int from_pid, uint32_t node, void* p);
	int (*read)(int fd, int from_pid, uint32_t node, void* buf, int size, int offset, void* p);
	int (*write)(int fd, int from_pid, uint32_t node, const void* buf, int size, int offset, void* p);
	int (*read_block)(int from_pid, void* buf, int size, int index, void* p);
	int (*write_block)(int from_pid, const void* buf, int size, int index, void* p);
	void* (*dma)(int fd, int from_pid, uint32_t node, int* size, void* p);
	int (*flush)(int fd, int from_pid, uint32_t node, void* p);
	int (*fcntl)(int fd, int from_pid, uint32_t node, int cmd, proto_t* in, proto_t* out, void* p);
	int (*mount)(fsinfo_t* mnt_point, void* p);
	int (*umount)(fsinfo_t* mnt_point, void* p);
	int (*unlink)(uint32_t node, const char *fname, void* p);
	int (*clear_buffer)(uint32_t node, void* p);
	void (*interrupt)(proto_t* in, void* p);
	void (*handled)(void* p);
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
