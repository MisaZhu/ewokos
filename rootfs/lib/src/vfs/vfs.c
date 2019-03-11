#include <vfs/vfs.h>
#include <vfs/vfscmd.h>
#include <kserv.h>
#include <proto.h>
#include <stdlib.h>
#include <pmessage.h>
#include <package.h>

#define VFS_NAME "kserv.vfsd"

inline uint32_t vfsAdd(uint32_t node, const char* name, uint32_t size) {
	int32_t servPid = kservGetPid(VFS_NAME);	
	if(servPid < 0)
		return 0;

	ProtoT* proto = protoNew(NULL, 0);
	protoAddInt(proto, (int32_t)node);
	protoAddStr(proto, name);
	protoAddInt(proto, (int32_t)size);

	PackageT* pkg = preq(servPid, 0, VFS_CMD_ADD, proto->data, proto->size);
	protoFree(proto);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return 0;
	}

	uint32_t ret = *(uint32_t*)getPackageData(pkg);
	free(pkg);
	return ret;
}

inline int32_t vfsDel(uint32_t node) {
	int32_t servPid = kservGetPid(VFS_NAME);	
	if(servPid < 0)
		return -1;

	PackageT* pkg = preq(servPid, 0, VFS_CMD_DEL, &node, 4);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}
	free(pkg);
	return 0;
}

inline int32_t vfsNodeInfo(uint32_t node, FSInfoT* info) {
	int32_t servPid = kservGetPid(VFS_NAME);	
	if(servPid < 0)
		return -1;

	PackageT* pkg = preq(servPid, 0, VFS_CMD_INFO, &node, 4);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}
	memcpy(info, getPackageData(pkg), sizeof(FSInfoT));
	free(pkg);
	return 0;
}

inline uint32_t vfsNodeByFD(int32_t fd) {
	int32_t servPid = kservGetPid(VFS_NAME);	
	if(servPid < 0)
		return -1;

	PackageT* pkg = preq(servPid, 0, VFS_CMD_NODE_BY_FD, &fd, 4);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}
	uint32_t node = *(uint32_t*)getPackageData(pkg);
	free(pkg);
	return node;
}

inline uint32_t vfsNodeByName(const char* fname) {
	int32_t servPid = kservGetPid(VFS_NAME);	
	if(servPid < 0)
		return 0;

	ProtoT* proto = protoNew(NULL, 0);
	protoAddStr(proto, fname);
	PackageT* pkg = preq(servPid, 0, VFS_CMD_NODE_BY_NAME, proto->data, proto->size);
	protoFree(proto);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return 0;
	}
	uint32_t node = *(uint32_t*)getPackageData(pkg);
	free(pkg);
	return node;
}

inline uint32_t vfsMount(const char* fname, const char* devName, int32_t devIndex, bool isFile) {
	int32_t servPid = kservGetPid(VFS_NAME);	
	if(servPid < 0)
		return 0;

	ProtoT* proto = protoNew(NULL, 0);
	protoAddStr(proto, fname);
	protoAddStr(proto, devName);
	protoAddInt(proto, devIndex);
	protoAddInt(proto, (int32_t)isFile);
	PackageT* pkg = preq(servPid, 0, VFS_CMD_MOUNT, proto->data, proto->size);
	protoFree(proto);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return 0;
	}
	uint32_t node = *(uint32_t*)getPackageData(pkg);
	free(pkg);
	return node;
}

inline int32_t vfsUnmount(uint32_t node) {
	int32_t servPid = kservGetPid(VFS_NAME);	
	if(servPid < 0)
		return -1;

	PackageT* pkg = preq(servPid, 0, VFS_CMD_UNMOUNT, &node, 4);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}
	free(pkg);
	return 0;

}

FSInfoT* vfsKids(uint32_t node, uint32_t* num) {
	*num = 0;
	int32_t servPid = kservGetPid(VFS_NAME);	
	if(servPid < 0)
		return NULL;

	PackageT* pkg = preq(servPid, 0, VFS_CMD_NODE_BY_FD, &node, 4);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR || pkg->size == 0) {
		if(pkg != NULL) free(pkg);
		return NULL;
	}

	FSInfoT* ret = (FSInfoT*)malloc(pkg->size);
	memcpy(ret, getPackageData(pkg), pkg->size);
	free(pkg);
	return ret;
}
