#include <kserv.h>
#include <unistd.h>
#include <stdio.h>
#include <ipc.h>
#include <proto.h>
#include <vfs/vfscmd.h>
#include <fstree.h>
#include <kstring.h>
#include <stdlib.h>
#include <syscall.h>

typedef struct {
	char devName[DEV_NAME_MAX];
	int32_t devIndex;
	int32_t devServPid;
	TreeNodeT* old;
} MountT;

#define DEV_VFS "dev.vfs"
#define MOUNT_MAX 32

static MountT _mounts[MOUNT_MAX];
static TreeNodeT _root;

static bool checkAccess(TreeNodeT* node, bool wr) {
	(void)node;
	(void)wr;
	return true;
}

static void fsnodeInit() {
	for(int i=0; i<MOUNT_MAX; i++) {
		memset(&_mounts[i], 0, sizeof(MountT));
	}
	strcpy(_mounts[0].devName, DEV_VFS);

	fsTreeNodeInit(&_root);
	strcpy(FSN(&_root)->name, "/");
}

static TreeNodeT* fsnodeAdd(TreeNodeT* nodeTo, const char* name, uint32_t size, int32_t pid) {
	int32_t owner = syscall1(SYSCALL_GET_UID, pid);
	if(!checkAccess(nodeTo, true))
		return NULL;

	TreeNodeT* ret = fsTreeSimpleGet(nodeTo, name);
	if(ret != NULL) 
		return ret;
	
	ret = fsTreeSimpleAdd(nodeTo, name);
	if(size > 0)
		FSN(ret)->type = FS_TYPE_FILE;
	else
		FSN(ret)->type = FS_TYPE_DIR;
	FSN(ret)->size = size;
	FSN(ret)->mount = FSN(nodeTo)->mount;
	FSN(ret)->owner = owner;
	return ret;
}

static int32_t fsnodeDel(TreeNodeT* node) {
	if(!checkAccess(node, true))
		return -1;
	treeDel(node, free);
	return 0;
}

static int32_t fsnodeNodeInfo(TreeNodeT* node, FSInfoT* info) {
	if(!checkAccess(node, false))
		return -1;
	
	if(FSN(node)->type == FS_TYPE_DIR)
		info->size = node->size;
	else 
		info->size = FSN(node)->size;
	
	info->id = node->id;
	info->node = (uint32_t)node;
	info->type = FSN(node)->type;
	info->owner = FSN(node)->owner;
	strcpy(info->devName, _mounts[FSN(node)->mount].devName);
	info->devIndex = _mounts[FSN(node)->mount].devIndex;
	info->devServPid = _mounts[FSN(node)->mount].devServPid;
	strcpy(info->name, FSN(node)->name);
	return 0;
}

TreeNodeT* getNodeByFD(int32_t pid, int32_t fd) {
	return (TreeNodeT*)syscall2(SYSCALL_PFILE_NODE, pid, fd);
}

TreeNodeT* getNodeByName(const char* fname) {
	return fsTreeGet(&_root, fname);
}

static TreeNodeT* buildNodes(const char* fname, int32_t owner) {
	TreeNodeT* father = &_root;
	if(fname[0] == '/')
		fname = fname+1;

	TreeNodeT* node = father;	
	char n[NAME_MAX+1];
	int j = 0;
	for(int i=0; i<NAME_MAX; i++) {
		n[i] = fname[i];
		if(n[i] == 0) {
			return fsnodeAdd(node, n+j, 0, owner);
		}
		if(n[i] == '/') {
			n[i] = 0; 
			node = fsnodeAdd(node, n+j, 0, owner);
			if(node == NULL)
				return NULL;
			j= i+1;
		}
	}
	return NULL;
}

static TreeNodeT* fsnodeMount(const char* fname, const char* devName, 
		int32_t devIndex, bool isFile, int32_t pid) {
	int32_t owner = syscall1(SYSCALL_GET_UID, pid);
	TreeNodeT* to = buildNodes(fname, owner);
	if(to == NULL)
		return NULL;

	int32_t i;
  for(i=0; i<MOUNT_MAX; i++) {
    if(_mounts[i].devName[0] == 0) {
			strcpy(_mounts[i].devName, devName);
			_mounts[i].devIndex = devIndex;
			_mounts[i].devServPid = pid;
			_mounts[i].old = to; //save the old node.
			break;
		}
	}
	if(i >= MOUNT_MAX)
		return NULL;

	TreeNodeT* node = fsNewNode();
	if(node == NULL)
		return NULL;

	FSN(node)->owner = owner;
  FSN(node)->mount = i;
	strcpy(FSN(node)->name, FSN(to)->name);

	//replace the old node
	node->father = to->father;
	node->prev = to->prev;
	if(node->prev != NULL)
		node->prev->next = node;
	if(node->next != NULL)
		node->next->prev = node;
	if(node->father->fChild == to)
		node->father->fChild = node;
	if(node->father->eChild == to)
		node->father->eChild = node;

	if(isFile)
		FSN(node)->type = FS_TYPE_FILE;
	else
		FSN(node)->type = FS_TYPE_DIR;
	return node;
}

static int32_t fsnodeUnmount(TreeNodeT* node) {
	if(!checkAccess(node, true))
		return -1;
	return 0;
}

static void doAdd(PackageT* pkg) {
	ProtoT* proto = protoNew(getPackageData(pkg), pkg->size);
	uint32_t node = (uint32_t)protoReadInt(proto);
	const char* name = protoReadStr(proto);
	uint32_t size = (uint32_t)protoReadInt(proto);

	TreeNodeT* ret = fsnodeAdd((TreeNodeT*)node, name, size, pkg->pid);
	ipcSend(pkg->id, pkg->type, &ret, 4);
}

static void doDel(PackageT* pkg) {
	uint32_t node = *(uint32_t*)getPackageData(pkg);
	if(fsnodeDel((TreeNodeT*)node) != 0)
		ipcSend(pkg->id, PKG_TYPE_ERR, NULL, 0);
	else
		ipcSend(pkg->id, pkg->type, NULL, 0);
}

static void doInfo(PackageT* pkg) {
	uint32_t node = *(uint32_t*)getPackageData(pkg);
	FSInfoT info;

	if(fsnodeNodeInfo((TreeNodeT*)node, &info) != 0)
		ipcSend(pkg->id, PKG_TYPE_ERR, NULL, 0);
	else
		ipcSend(pkg->id, pkg->type, &info, sizeof(FSInfoT));
}

static void doNodeByFD(PackageT* pkg) {
	int32_t fd = *(int32_t*)getPackageData(pkg);

	TreeNodeT* node = getNodeByFD(pkg->pid, fd);
	ipcSend(pkg->id, pkg->type, &node, 4);
}

static void doNodeByName(PackageT* pkg) {
	ProtoT* proto = protoNew(getPackageData(pkg), pkg->size);
	const char* name = protoReadStr(proto);
	protoFree(proto);

	TreeNodeT* node = getNodeByName(name);
	ipcSend(pkg->id, pkg->type, &node, 4);
}

static void doKids(PackageT* pkg) {
	TreeNodeT* node = (TreeNodeT*)(*(int32_t*)getPackageData(pkg));

	if(node == NULL || node->size == 0) {
		ipcSend(pkg->id, pkg->type, NULL, 0);
		return;
	}

	uint32_t size = sizeof(FSInfoT) * node->size;
	FSInfoT* ret = (FSInfoT*)malloc(size);
	memset(ret, 0, size);

	uint32_t i;
	TreeNodeT* n = node->fChild;
	for(i=0; i<node->size; i++) {
		if(n == NULL)
			break;
		fsnodeNodeInfo(n, &ret[i]);
		n = n->next;
	}
	ipcSend(pkg->id, pkg->type, ret, size);
	free(ret);
}

static void doMount(PackageT* pkg) {
	ProtoT* proto = protoNew(getPackageData(pkg), pkg->size);
	const char* fname = protoReadStr(proto);
	const char* devName = protoReadStr(proto);
	int32_t devIndex = protoReadInt(proto);
	int32_t isFile = protoReadInt(proto);
	protoFree(proto);

	TreeNodeT* node = fsnodeMount(fname, devName, devIndex, isFile, pkg->pid);
	ipcSend(pkg->id, pkg->type, &node, 4);
}

static void doUnmount(PackageT* pkg) {
	TreeNodeT* node = (TreeNodeT*)(*(int32_t*)getPackageData(pkg));
	fsnodeUnmount(node);
	ipcSend(pkg->id, pkg->type, NULL, 0);
}

static void handle(PackageT* pkg, void* p) {
	(void)p;
	switch(pkg->type) {
	case VFS_CMD_ADD:
		doAdd(pkg);
		break;
	case VFS_CMD_DEL:
		doDel(pkg);
		break;
	case VFS_CMD_INFO:
		doInfo(pkg);
		break;
	case VFS_CMD_NODE_BY_FD:
		doNodeByFD(pkg);
		break;
	case VFS_CMD_NODE_BY_NAME:
		doNodeByName(pkg);
		break;
	case VFS_CMD_KIDS:
		doKids(pkg);
		break;
	case VFS_CMD_MOUNT:
		doMount(pkg);
		break;
	case VFS_CMD_UNMOUNT:
		doUnmount(pkg);
		break;
	}
}

void _start() {
	if(kservGetPid("kserv.vfsd") >= 0) {
    printf("Panic: 'kserv.vfsd' process has been running already!\n");
		exit(0);
	}

	fsnodeInit();
	if(!kservRun("kserv.vfsd", handle, NULL)) {
		exit(0);
	}
}
