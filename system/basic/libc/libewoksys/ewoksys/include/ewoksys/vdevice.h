#ifndef VDEVICE_H
#define VDEVICE_H

#include <ewoksys/fsinfo.h>
#include <ewoksys/proto.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VFS_ERR_RETRY (-128)

#define MAX_TRUST_PID 10

typedef struct st_vdevice {
	bool terminated;
	char name[FS_NODE_NAME_MAX];
	fsinfo_t mnt_info;
	void* extra_data;
	int (*dev_cntl)(struct st_vdevice* dev, int from_pid, int cmd, proto_t* in, proto_t* ret, void* p);
	int (*open)(struct st_vdevice* dev, int fd, int from_pid, fsinfo_t* info, int oflag, void* p);
	int (*stat)(struct st_vdevice* dev, int from_pid, fsinfo_t* info, node_stat_t* stat, void* p);
	int (*create)(struct st_vdevice* dev, int from_pid, fsinfo_t *info_to, fsinfo_t* info, void* p);
	int (*close)(struct st_vdevice* dev, int fd, int from_pid, uint32_t node, fsinfo_t* fsinfo, void* p);
	int (*read)(struct st_vdevice* dev, int fd, int from_pid, fsinfo_t* info, void* buf, int size, int offset, void* p);
	int (*write)(struct st_vdevice* dev, int fd, int from_pid, fsinfo_t* info, const void* buf, int size, int offset, void* p);
	int (*read_block)(struct st_vdevice* dev, int from_pid, void* buf, int size, int index, void* p);
	int (*write_block)(struct st_vdevice* dev, int from_pid, const void* buf, int size, int index, void* p);
	int32_t (*dma)(struct st_vdevice* dev, int fd, int from_pid, fsinfo_t* fsinfo, int* size, void* p);
	int (*flush)(struct st_vdevice* dev, int fd, int from_pid, fsinfo_t* fsinfo, void* p);
	int (*fcntl)(struct st_vdevice* dev, int fd, int from_pid, fsinfo_t* info, int cmd, proto_t* in, proto_t* out, void* p);
	int (*set)(struct st_vdevice* dev, int from_pid, fsinfo_t* info, void* p);
	int (*get)(struct st_vdevice* dev, int from_pid, const char* fname, fsinfo_t* info, void* p);
	fsinfo_t* (*kids)(struct st_vdevice* dev, fsinfo_t* info_dir, uint32_t* num, void* p);
	char* (*cmd)(struct st_vdevice* dev, int from_pid, int argc, char**argv, void* p);
	int (*mount)(struct st_vdevice* dev, fsinfo_t* mnt_point, void* p);
	int (*umount)(struct st_vdevice* dev, uint32_t node, void* p);
	int (*unlink)(struct st_vdevice* dev, fsinfo_t* info, const char* fname, void* p);
	int (*clear_buffer)(struct st_vdevice* dev, uint32_t node, void* p);
	void (*interrupt)(struct st_vdevice* dev, proto_t* in, void* p);
	void (*handled)(struct st_vdevice* dev, void* p);
	int (*loop_step)(struct st_vdevice* dev, void* p);
} vdevice_t;

extern int device_run(vdevice_t* dev, const char* mnt_point, int mnt_type, int mode);
extern void device_stop(vdevice_t* dev);

extern int dev_get_pid(const char* fname);

extern char* dev_cmd_by_pid(int pid, const char* cmd);
extern char* dev_cmd(const char* fname, const char* cmd);

extern int dev_cntl(const char* fname, int cmd, proto_t* in, proto_t* out);
extern int dev_cntl_by_pid(int pid, int cmd, proto_t* in, proto_t* out);

extern fsinfo_t*  dev_get_file(int fd, int pid, uint32_t node);
extern int dev_update_file(int fd, int from_pid, fsinfo_t* finfo);

#ifdef __cplusplus
}
#endif

#endif
