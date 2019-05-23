#include <vfs/vfs.h>
#include <vfs/vfscmd.h>
#include <kserv.h>
#include <proto.h>
#include <stdlib.h>
#include <ipc.h>
#include <package.h>
#include <syscall.h>

inline int32_t vfs_add(const char* dir_name, const char* name, uint32_t size, void* data) {
	int32_t serv_pid = kserv_get_by_name(VFS_NAME);	
	if(serv_pid < 0)
		return -1;

	proto_t* in = proto_new(NULL, 0);
	proto_add_str(in, dir_name);
	proto_add_str(in, name);
	proto_add_int(in, (int32_t)size);
	proto_add_int(in, (int32_t)data);

	int32_t ret = ipc_call(serv_pid, VFS_CMD_ADD, in, NULL);
	proto_free(in);
	return ret;
}

inline int32_t vfs_del(const char* fname) {
	int32_t serv_pid = kserv_get_by_name(VFS_NAME);	
	if(serv_pid < 0)
		return -1;

	proto_t* in = proto_new(NULL, 0);
	proto_add_str(in, fname);
	int32_t ret = ipc_call(serv_pid, VFS_CMD_DEL, in, NULL);
	proto_free(in);
	return ret;
}

inline int32_t vfs_open(const char* fname, int32_t flags) {
	int32_t serv_pid = kserv_get_by_name(VFS_NAME);	
	if(serv_pid < 0)
		return -1;

	proto_t* in = proto_new(NULL, 0);
	proto_add_str(in, fname);
	proto_add_int(in, flags);
	proto_t* out = proto_new(NULL, 0);
	int32_t ret = ipc_call(serv_pid, VFS_CMD_OPEN, in, out);
	proto_free(in);
	if(ret == 0)
		ret = proto_read_int(out);
	proto_free(out);
	return ret;
}

inline int32_t vfs_close(int32_t fd) {
	int32_t serv_pid = kserv_get_by_name(VFS_NAME);	
	if(serv_pid < 0)
		return -1;
	proto_t* in = proto_new(NULL, 0);
	proto_add_int(in, fd);
	ipc_call(serv_pid, VFS_CMD_CLOSE, in, NULL);
	proto_free(in);
	return 0;
}

inline int32_t vfs_info_by_name(const char* fname, fs_info_t* info) {
	if(fname[0] == 0)
		return -1;
	int32_t serv_pid = kserv_get_by_name(VFS_NAME);	
	if(serv_pid < 0)
		return -1;

	proto_t* in = proto_new(NULL, 0);
	proto_t* out = proto_new(NULL, 0);
	proto_add_str(in, fname);
	int32_t res = ipc_call(serv_pid, VFS_CMD_INFO_BY_NAME, in, out);
	proto_free(in);
	if(res == 0)
		memcpy(info, proto_read(out, NULL), sizeof(fs_info_t));
	proto_free(out);
	return res;
}

inline int32_t vfs_info_by_fd(int32_t fd, fs_info_t* info) {
	if(fd < 0)
		return -1;
	int32_t serv_pid = kserv_get_by_name(VFS_NAME);	
	if(serv_pid < 0)
		return -1;
	proto_t* in = proto_new(NULL, 0);
	proto_t* out = proto_new(NULL, 0);
	proto_add_int(in, fd);
	int32_t res = ipc_call(serv_pid, VFS_CMD_INFO_BY_FD, in, out);
	if(res == 0)
		memcpy(info, proto_read(out, NULL), sizeof(fs_info_t));
	proto_free(out);
	return res;
}

inline int32_t vfs_pipe_open(int32_t fds[2]) {
	int32_t serv_pid = kserv_get_by_name(VFS_NAME);	
	if(serv_pid < 0)
		return -1;

	proto_t* out = proto_new(NULL, 0);
	int32_t res = ipc_call(serv_pid, VFS_CMD_PIPE_OPEN, NULL, out);
	if(res == 0) {
		fds[0] = proto_read_int(out);
		fds[1] = proto_read_int(out);
	}
	proto_free(out);
	return res;
}

inline int32_t vfs_node_update(fs_info_t* info) {
	int32_t serv_pid = kserv_get_by_name(VFS_NAME);	
	if(serv_pid < 0)
		return -1;

	proto_t* in = proto_new(NULL, 0);
	proto_add(in, &info, sizeof(fs_info_t));
	int32_t res = ipc_call(serv_pid, VFS_CMD_INFO_UPDATE, in, NULL);
	proto_free(in);
	return res;
}

inline int32_t vfs_mount(const char* fname, const char* devName, int32_t devIndex, bool isFile) {
	int32_t serv_pid = kserv_get_by_name(VFS_NAME);	
	if(serv_pid < 0)
		return -1;

	proto_t* in = proto_new(NULL, 0);
	proto_add_str(in, fname);
	proto_add_str(in, devName);
	proto_add_int(in, devIndex);
	proto_add_int(in, (int32_t)isFile);
	int32_t res = ipc_call(serv_pid, VFS_CMD_MOUNT, in, NULL);
	proto_free(in);
	return res;
}

inline int32_t vfs_unmount(const char* fname) {
	int32_t serv_pid = kserv_get_by_name(VFS_NAME);	
	if(serv_pid < 0)
		return -1;

	proto_t* in = proto_new(NULL, 0);
	proto_add_str(in, fname);
	int32_t res = ipc_call(serv_pid, VFS_CMD_MOUNT, in, NULL);
	proto_free(in);
	return res;
}

tstr_t* vfs_kid(const char* dir_name, int32_t index) {
	int32_t serv_pid = kserv_get_by_name(VFS_NAME);	
	if(serv_pid < 0)
		return NULL;

	proto_t* in = proto_new(NULL, 0);
	proto_t* out = proto_new(NULL, 0);
	proto_add_str(in, dir_name);
	proto_add_int(in, index);
	int32_t res = ipc_call(serv_pid, VFS_CMD_KID, in, out);
	proto_free(in);
	tstr_t *ret = NULL;
	if(res == 0)
		ret = tstr_new(proto_read_str(out), MFS);
	proto_free(out);
	return ret;
}

int32_t vfs_mount_by_index(int32_t index, mount_t* mnt) {
	int32_t serv_pid = kserv_get_by_name(VFS_NAME);	
	if(serv_pid < 0)
		return -1;

	proto_t* in = proto_new(NULL, 0);
	proto_t* out = proto_new(NULL, 0);
	proto_add_int(in, index);
	int32_t res = ipc_call(serv_pid, VFS_CMD_MOUNT_BY_INDEX, in, out);
	proto_free(in);
	if(res == 0)
		memcpy(mnt, proto_read(out, NULL), sizeof(mount_t));
	proto_free(out);
	return res;
}

int32_t vfs_mount_by_fname(const char* fname, mount_t* mnt) {
	int32_t serv_pid = kserv_get_by_name(VFS_NAME);	
	if(serv_pid < 0)
		return -1;

	proto_t* in = proto_new(NULL, 0);
	proto_t* out = proto_new(NULL, 0);
	proto_add_str(in, fname);
	int32_t res = ipc_call(serv_pid, VFS_CMD_MOUNT_BY_FNAME, in, out);
	proto_free(in);
	if(res == 0)
		memcpy(mnt, proto_read(out, NULL), sizeof(mount_t));
	proto_free(out);
	return res;
}

//get the full name by fd
tstr_t* vfs_full_name_by_fd(int32_t fd) {
	if(fd < 0)
		return NULL;
	int32_t serv_pid = kserv_get_by_name(VFS_NAME);	
	if(serv_pid < 0)
		return NULL;

	proto_t* in = proto_new(NULL, 0);
	proto_t* out = proto_new(NULL, 0);
	proto_add_int(in, fd);
	int32_t res = ipc_call(serv_pid, VFS_CMD_FULLNAME_BY_FD, in, NULL);
	proto_free(in);
	
	tstr_t *ret = NULL;
	if(res == 0)
		tstr_new(proto_read_str(out), MFS);
	proto_free(out);
	return ret;
}

//get the full name by node
tstr_t* vfs_full_name_by_node(uint32_t node) {
	if(node == 0)
		return NULL;
	int32_t serv_pid = kserv_get_by_name(VFS_NAME);	
	if(serv_pid < 0)
		return NULL;

	proto_t* in = proto_new(NULL, 0);
	proto_t* out = proto_new(NULL, 0);
	proto_add_int(in, node);
	int32_t res = ipc_call(serv_pid, VFS_CMD_FULLNAME_BY_NODE, in, NULL);
	proto_free(in);
	
	tstr_t *ret = NULL;
	if(res == 0)
		tstr_new(proto_read_str(out), MFS);
	proto_free(out);
	return ret;
}

//get the short name by fd.
tstr_t* vfs_short_name_by_fd(int32_t fd) {
	if(fd < 0)
		return NULL;
	int32_t serv_pid = kserv_get_by_name(VFS_NAME);	
	if(serv_pid < 0)
		return NULL;

	proto_t* in = proto_new(NULL, 0);
	proto_t* out = proto_new(NULL, 0);
	proto_add_int(in, fd);
	int32_t res = ipc_call(serv_pid, VFS_CMD_SHORTNAME_BY_FD, in, NULL);
	proto_free(in);
	
	tstr_t *ret = NULL;
	if(res == 0)
		tstr_new(proto_read_str(out), MFS);
	proto_free(out);
	return ret;
}

//get the full name by node
tstr_t* vfs_short_name_by_node(uint32_t node) {
	if(node == 0)
		return NULL;
	int32_t serv_pid = kserv_get_by_name(VFS_NAME);	
	if(serv_pid < 0)
		return NULL;

	proto_t* in = proto_new(NULL, 0);
	proto_t* out = proto_new(NULL, 0);
	proto_add_int(in, node);
	int32_t res = ipc_call(serv_pid, VFS_CMD_SHORTNAME_BY_NODE, in, NULL);
	proto_free(in);
	
	tstr_t *ret = NULL;
	if(res == 0)
		tstr_new(proto_read_str(out), MFS);
	proto_free(out);
	return ret;
}
