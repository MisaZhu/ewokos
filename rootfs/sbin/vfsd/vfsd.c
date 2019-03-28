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
	tree_node_t* old;
} _mountT;

#define DEV_VFS "dev.vfs"
#define MOUNT_MAX 32

static _mountT _mounts[MOUNT_MAX];
static tree_node_t _root;

static bool check_access(tree_node_t* node, bool wr) {
	(void)node;
	(void)wr;
	return true;
}

static void fsnodeInit() {
	for(int i=0; i<MOUNT_MAX; i++) {
		memset(&_mounts[i], 0, sizeof(_mountT));
	}
	strcpy(_mounts[0].devName, DEV_VFS);

	fsTreeNodeInit(&_root);
	strcpy(FSN(&_root)->name, "/");
}

static tree_node_t* fsnode_add(tree_node_t* nodeTo, const char* name, uint32_t size, int32_t pid) {
	int32_t owner = syscall1(SYSCALL_GET_UID, pid);
	if(!check_access(nodeTo, true))
		return NULL;

	tree_node_t* ret = fsTreeSimpleGet(nodeTo, name);
	if(ret != NULL) 
		return ret;
	
	ret = fsTreeSimpleAdd(nodeTo, name);
	if(size != 0xffffff)
		FSN(ret)->type = FS_TYPE_FILE;
	else
		FSN(ret)->type = FS_TYPE_DIR;
	FSN(ret)->size = size;
	FSN(ret)->mount = FSN(nodeTo)->mount;
	FSN(ret)->owner = owner;
	return ret;
}

static int32_t fsnode_del(tree_node_t* node) {
	if(!check_access(node, true))
		return -1;
	tree_del(node, free);
	return 0;
}

static int32_t fsnode_node_info(tree_node_t* node, fs_info_t* info) {
	if(!check_access(node, false))
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

static tree_node_t* get_node_by_fd(int32_t pid, int32_t fd) {
	return (tree_node_t*)syscall2(SYSCALL_PFILE_NODE, pid, fd);
}

static tree_node_t* get_node_by_name(const char* fname) {
	return fsTreeGet(&_root, fname);
}

static tree_node_t* build_nodes(const char* fname, int32_t owner) {
	tree_node_t* father = &_root;
	if(fname[0] == '/')
		fname = fname+1;

	tree_node_t* node = father;	
	char n[NAME_MAX+1];
	int j = 0;
	for(int i=0; i<NAME_MAX; i++) {
		n[i] = fname[i];
		if(n[i] == 0) {
			return fsnode_add(node, n+j, 0xffffff, owner);
		}
		if(n[i] == '/') {
			n[i] = 0; 
			node = fsnode_add(node, n+j, 0xffffff, owner);
			if(node == NULL)
				return NULL;
			j= i+1;
		}
	}
	return NULL;
}

static tree_node_t* fsnode_mount(const char* fname, const char* devName, 
		int32_t devIndex, bool isFile, int32_t pid) {
	int32_t owner = syscall1(SYSCALL_GET_UID, pid);
	tree_node_t* to = build_nodes(fname, owner);
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

	tree_node_t* node = fsNewNode();
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

static int32_t fsnode_unmount(tree_node_t* node) {
	if(!check_access(node, true))
		return -1;
	tree_node_t* old = _mounts[FSN(node)->mount].old;
	tree_node_t* father = node->father;
	tree_del(node, free);

	if(old != NULL && father != NULL)
		tree_add(father, old);
	return 0;
}

static void do_add(package_t* pkg) {
	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	uint32_t node = (uint32_t)proto_read_int(proto);
	const char* name = proto_read_str(proto);
	uint32_t size = (uint32_t)proto_read_int(proto);

	tree_node_t* ret = fsnode_add((tree_node_t*)node, name, size, pkg->pid);
	ipc_send(pkg->id, pkg->type, &ret, 4);
}

static void do_del(package_t* pkg) {
	uint32_t node = *(uint32_t*)get_pkg_data(pkg);
	if(fsnode_del((tree_node_t*)node) != 0)
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
	else
		ipc_send(pkg->id, pkg->type, NULL, 0);
}

static void do_info(package_t* pkg) {
	uint32_t node = *(uint32_t*)get_pkg_data(pkg);
	fs_info_t info;

	if(node == 0 || fsnode_node_info((tree_node_t*)node, &info) != 0)
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
	else
		ipc_send(pkg->id, pkg->type, &info, sizeof(fs_info_t));
}

static void do_node_by_fd(package_t* pkg) {
	int32_t fd = *(int32_t*)get_pkg_data(pkg);

	tree_node_t* node = get_node_by_fd(pkg->pid, fd);
	ipc_send(pkg->id, pkg->type, &node, 4);
}

static void do_node_by_name(package_t* pkg) {
	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	const char* name = proto_read_str(proto);
	proto_free(proto);

	tree_node_t* node = get_node_by_name(name);
	ipc_send(pkg->id, pkg->type, &node, 4);
}

static void do_kids(package_t* pkg) {
	tree_node_t* node = (tree_node_t*)(*(int32_t*)get_pkg_data(pkg));

	if(node == NULL || node->size == 0) {
		ipc_send(pkg->id, pkg->type, NULL, 0);
		return;
	}

	uint32_t size = sizeof(fs_info_t) * node->size;
	fs_info_t* ret = (fs_info_t*)malloc(size);
	memset(ret, 0, size);

	uint32_t i;
	tree_node_t* n = node->fChild;
	for(i=0; i<node->size; i++) {
		if(n == NULL)
			break;
		fsnode_node_info(n, &ret[i]);
		n = n->next;
	}
	ipc_send(pkg->id, pkg->type, ret, size);
	free(ret);
}

static void do_mount(package_t* pkg) {
	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	const char* fname = proto_read_str(proto);
	const char* devName = proto_read_str(proto);
	int32_t devIndex = proto_read_int(proto);
	int32_t isFile = proto_read_int(proto);
	proto_free(proto);

	tree_node_t* node = fsnode_mount(fname, devName, devIndex, isFile, pkg->pid);
	ipc_send(pkg->id, pkg->type, &node, 4);
}

static void do_unmount(package_t* pkg) {
	tree_node_t* node = (tree_node_t*)(*(int32_t*)get_pkg_data(pkg));
	fsnode_unmount(node);
	ipc_send(pkg->id, pkg->type, NULL, 0);
}

static void handle(package_t* pkg, void* p) {
	(void)p;
	switch(pkg->type) {
	case VFS_CMD_ADD:
		do_add(pkg);
		break;
	case VFS_CMD_DEL:
		do_del(pkg);
		break;
	case VFS_CMD_INFO:
		do_info(pkg);
		break;
	case VFS_CMD_NODE_BY_FD:
		do_node_by_fd(pkg);
		break;
	case VFS_CMD_NODE_BY_NAME:
		do_node_by_name(pkg);
		break;
	case VFS_CMD_KIDS:
		do_kids(pkg);
		break;
	case VFS_CMD_MOUNT:
		do_mount(pkg);
		break;
	case VFS_CMD_UNMOUNT:
		do_unmount(pkg);
		break;
	}
}

int main() {
	if(kserv_get_pid("kserv.vfsd") >= 0) {
    printf("panic: 'kserv.vfsd' process has been running already!\n");
		return -1;
	}

	fsnodeInit();
	if(!kserv_run("kserv.vfsd", handle, NULL)) {
		return -1;
	}
	return 0;
}
