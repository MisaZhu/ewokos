#include <vfs/vfs.h>
#include <vfs/vfscmd.h>
#include <kserv.h>
#include <proto.h>
#include <stdlib.h>
#include <ipc.h>
#include <package.h>

#define VFS_NAME "kserv.vfsd"

inline uint32_t vfs_add(uint32_t node, const char* name, uint32_t size, void* data) {
	int32_t servPid = kserv_get_pid(VFS_NAME);	
	if(servPid < 0)
		return 0;

	proto_t* proto = proto_new(NULL, 0);
	proto_add_int(proto, (int32_t)node);
	proto_add_str(proto, name);
	proto_add_int(proto, (int32_t)size);
	proto_add_int(proto, (int32_t)data);

	package_t* pkg = ipc_req(servPid, 0, VFS_CMD_ADD, proto->data, proto->size, true);
	proto_free(proto);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return 0;
	}

	uint32_t ret = *(uint32_t*)get_pkg_data(pkg);
	free(pkg);
	return ret;
}

inline int32_t vfs_del(uint32_t node) {
	int32_t servPid = kserv_get_pid(VFS_NAME);	
	if(servPid < 0)
		return -1;

	package_t* pkg = ipc_req(servPid, 0, VFS_CMD_DEL, &node, 4, true);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}
	free(pkg);
	return 0;
}

inline int32_t vfs_node_info(uint32_t node, fs_info_t* info) {
	int32_t servPid = kserv_get_pid(VFS_NAME);	
	if(servPid < 0)
		return -1;

	package_t* pkg = ipc_req(servPid, 0, VFS_CMD_INFO, &node, 4, true);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}
	memcpy(info, get_pkg_data(pkg), sizeof(fs_info_t));
	free(pkg);
	return 0;
}

inline uint32_t vfs_node_by_fd(int32_t fd) {
	int32_t servPid = kserv_get_pid(VFS_NAME);	
	if(servPid < 0)
		return -1;

	package_t* pkg = ipc_req(servPid, 0, VFS_CMD_NODE_BY_FD, &fd, 4, true);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}
	uint32_t node = *(uint32_t*)get_pkg_data(pkg);
	free(pkg);
	return node;
}

inline uint32_t vfs_node_by_name(const char* fname) {
	int32_t servPid = kserv_get_pid(VFS_NAME);	
	if(servPid < 0)
		return 0;

	proto_t* proto = proto_new(NULL, 0);
	proto_add_str(proto, fname);
	package_t* pkg = ipc_req(servPid, 0, VFS_CMD_NODE_BY_NAME, proto->data, proto->size, true);
	proto_free(proto);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return 0;
	}
	uint32_t node = *(uint32_t*)get_pkg_data(pkg);
	free(pkg);
	return node;
}

inline uint32_t vfs_mount(const char* fname, const char* devName, int32_t devIndex, bool isFile) {
	int32_t servPid = kserv_get_pid(VFS_NAME);	
	if(servPid < 0)
		return 0;

	proto_t* proto = proto_new(NULL, 0);
	proto_add_str(proto, fname);
	proto_add_str(proto, devName);
	proto_add_int(proto, devIndex);
	proto_add_int(proto, (int32_t)isFile);
	package_t* pkg = ipc_req(servPid, 0, VFS_CMD_MOUNT, proto->data, proto->size, true);
	proto_free(proto);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return 0;
	}
	uint32_t node = *(uint32_t*)get_pkg_data(pkg);
	free(pkg);
	return node;
}

inline int32_t vfs_unmount(uint32_t node) {
	int32_t servPid = kserv_get_pid(VFS_NAME);	
	if(servPid < 0)
		return -1;

	package_t* pkg = ipc_req(servPid, 0, VFS_CMD_UNMOUNT, &node, 4, true);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}
	free(pkg);
	return 0;

}

fs_info_t* vfs_kids(uint32_t node, uint32_t* num) {
	*num = 0;
	int32_t servPid = kserv_get_pid(VFS_NAME);	
	if(servPid < 0)
		return NULL;

	package_t* pkg = ipc_req(servPid, 0, VFS_CMD_KIDS, &node, 4, true);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR || pkg->size == 0) {
		if(pkg != NULL) free(pkg);
		return NULL;
	}

	fs_info_t* ret = (fs_info_t*)malloc(pkg->size);
	memcpy(ret, get_pkg_data(pkg), pkg->size);
	*num = pkg->size / sizeof(fs_info_t);
	free(pkg);
	return ret;
}
