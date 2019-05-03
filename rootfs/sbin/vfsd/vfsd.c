#include <kserv.h>
#include <unistd.h>
#include <stdio.h>
#include <ipc.h>
#include <proto.h>
#include <vfs/vfs.h>
#include <vfs/vfscmd.h>
#include <fstree.h>
#include <kstring.h>
#include <stdlib.h>
#include <syscall.h>

#define DEV_VFS "dev.vfs"
#define MOUNT_MAX 32

static mount_t _mounts[MOUNT_MAX];
static tree_node_t* _root = NULL;

static bool check_access(tree_node_t* node, bool wr) {
	(void)wr;
	if(node == NULL)
		return false;
	return true;
}

static void fsnode_init() {
	for(int i=0; i<MOUNT_MAX; i++) {
		memset(&_mounts[i], 0, sizeof(mount_t));
	}
	//strcpy(_mounts[0].dev_name, DEV_VFS);
	_root = NULL;
}

static tree_node_t* fsnode_add(tree_node_t* node_to, const char* name, uint32_t size, int32_t pid, void* data) {
	int32_t owner = syscall1(SYSCALL_GET_UID, pid);
	if(!check_access(node_to, true))
		return NULL;

	tree_node_t* ret = fs_tree_simple_get(node_to, name);
	if(ret != NULL) 
		return ret;
	
	ret = fs_tree_simple_add(node_to, name);
	if(size != VFS_DIR_SIZE)
		FSN(ret)->type = FS_TYPE_FILE;
	else
		FSN(ret)->type = FS_TYPE_DIR;
	FSN(ret)->size = size;
	FSN(ret)->mount = FSN(node_to)->mount;
	FSN(ret)->owner = owner;
	FSN(ret)->data = data;
	return ret;
}

static int32_t fsnode_del(tree_node_t* node) {
	if(!check_access(node, true))
		return -1;
	tree_del(node, free);
	return 0;
}

static int32_t fsnode_info(tree_node_t* node, fs_info_t* info) {
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
	info->dev_index = _mounts[FSN(node)->mount].dev_index;
	info->dev_serv_pid = _mounts[FSN(node)->mount].dev_serv_pid;
	strcpy(info->name, FSN(node)->name);
	info->data = FSN(node)->data;
	return 0;
}

static tree_node_t* get_node_by_name(const char* fname) {
	return fs_tree_get(_root, fname);
}

static tree_node_t* build_nodes(const char* fname, int32_t owner) {
	if(_root == NULL)
		return NULL;

	tree_node_t* father = _root;
	if(fname[0] == '/')
		fname = fname+1;

	tree_node_t* node = father;	
	char n[FULL_NAME_MAX+1];
	int j = 0;
	for(int i=0; i<FULL_NAME_MAX; i++) {
		n[i] = fname[i];
		if(n[i] == 0) {
			if(i == 0)
				return node;
			else
				return fsnode_add(node, n+j, VFS_DIR_SIZE, owner, NULL);
		}
		if(n[i] == '/') {
			n[i] = 0; 
			node = fsnode_add(node, n+j, VFS_DIR_SIZE, owner, NULL);
			if(node == NULL)
				return NULL;
			j= i+1;
		}
	}
	return NULL;
}

static tree_node_t* fsnode_mount(const char* fname, const char* dev_name, 
		int32_t dev_index, bool isFile, int32_t pid) {
	int32_t owner = syscall1(SYSCALL_GET_UID, pid);
	tree_node_t* to = build_nodes(fname, owner);

	int32_t i;
  for(i=0; i<MOUNT_MAX; i++) {
    if(_mounts[i].dev_name[0] == 0) {
			strcpy(_mounts[i].dev_name, dev_name);
			_mounts[i].dev_index = dev_index;
			_mounts[i].dev_serv_pid = pid;
			_mounts[i].node_old = (uint32_t)to; //save the node_old node.
			break;
		}
	}
	if(i >= MOUNT_MAX)
		return NULL;

	tree_node_t* node = fs_new_node();
	if(node == NULL)
		return NULL;

	if(to == NULL || _root == NULL) {
		_root = node;
		strcpy(FSN(node)->name, "/");
	}
	else
		strcpy(FSN(node)->name, FSN(to)->name);

	FSN(node)->owner = owner;
  FSN(node)->mount = i;

	//replace the node_old node
	if(to != NULL) {
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
	}

	if(isFile)
		FSN(node)->type = FS_TYPE_FILE;
	else
		FSN(node)->type = FS_TYPE_DIR;
	return node;
}

static int32_t fsnode_unmount(tree_node_t* node) {
	if(!check_access(node, true) || node == _root) //can not umount root
		return -1;
	int32_t index = FSN(node)->mount;

	tree_node_t* node_old = (tree_node_t*)_mounts[FSN(node)->mount].node_old;
	tree_node_t* father = node->father;
	tree_del(node, free);

	if(node_old != NULL && father != NULL)
		tree_add(father, node_old);
	return 0;
}

static void do_add(package_t* pkg) {
	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	uint32_t node = (uint32_t)proto_read_int(proto);
	const char* name = proto_read_str(proto);
	uint32_t size = (uint32_t)proto_read_int(proto);
	void* data = (void*)proto_read_int(proto);

	tree_node_t* ret = NULL; 
	if(strchr(name, '/') == NULL)
		ret = fsnode_add((tree_node_t*)node, name, size, pkg->pid, data);
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

	if(node == 0 || fsnode_info((tree_node_t*)node, &info) != 0)
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
	else
		ipc_send(pkg->id, pkg->type, &info, sizeof(fs_info_t));
}

static void do_info_update(package_t* pkg) {
	fs_info_t* info = (fs_info_t*)get_pkg_data(pkg);

	if(info == NULL) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	tree_node_t* node = (tree_node_t*)info->node;
	if(pkg->pid != _mounts[FSN(node)->mount].dev_serv_pid) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}
	FSN(node)->size = info->size;
	FSN(node)->data = info->data;
	FSN(node)->owner = info->owner;
	strcpy(FSN(node)->name, info->name);
	ipc_send(pkg->id, pkg->type, NULL, 0);
}

static void do_node_by_name(package_t* pkg) {
	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	const char* name = proto_read_str(proto);
	proto_free(proto);

	fs_info_t info;
	tree_node_t* node = get_node_by_name(name);
	if(node == NULL || fsnode_info(node, &info) != 0)
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
	else
		ipc_send(pkg->id, pkg->type, &info, sizeof(fs_info_t));
}

static void do_node_fullname(package_t* pkg) {
	uint32_t addr = *(uint32_t*)get_pkg_data(pkg);
	tree_node_t* node = (tree_node_t*)addr;
	fs_info_t info;

	if(node == NULL || fsnode_info(node, &info) != 0) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}
	char full[FULL_NAME_MAX] = {0};
	int32_t i = FULL_NAME_MAX-2;
	const char* n;
	while(i > 0) {
		if(node->father == NULL)
			break;
		n = FSN(node)->name;
		int32_t j = strlen(n) - 1;
		while(i > 0 && j >= 0) {
			full[i--] = n[j--];
		}
		full[i--] = '/';
		node = node->father;
	}
	n = full+i+1;
	ipc_send(pkg->id, pkg->type, (void*)n, strlen(n)+1);
}

static void do_kid(package_t* pkg) {
	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	uint32_t addr = (uint32_t)proto_read_int(proto);
	int32_t index = proto_read_int(proto);
	proto_free(proto);
	tree_node_t* node = (tree_node_t*)addr;

	if(node == NULL || index < 0 || index >= (int32_t)node->size) {
		ipc_send(pkg->id, pkg->type, NULL, 0);
		return;
	}

	int32_t i;
	tree_node_t* n = node->fChild;
	for(i=0; i<index; i++) {
		if(n == NULL)
			break;
		n = n->next;
	}
	if(n == NULL) {
		ipc_send(pkg->id, pkg->type, NULL, 0);
		return;
	}
	fs_info_t info;
	fsnode_info(n, &info);
	ipc_send(pkg->id, pkg->type, &info, sizeof(fs_info_t));
}

static void do_mount(package_t* pkg) {
	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	const char* fname = proto_read_str(proto);
	const char* dev_name = proto_read_str(proto);
	int32_t dev_index = proto_read_int(proto);
	int32_t isFile = proto_read_int(proto);
	proto_free(proto);

	tree_node_t* node = fsnode_mount(fname, dev_name, dev_index, isFile, pkg->pid);
	ipc_send(pkg->id, pkg->type, &node, 4);
}

static void do_unmount(package_t* pkg) {
	tree_node_t* node = (tree_node_t*)(*(int32_t*)get_pkg_data(pkg));
	fsnode_unmount(node);
	ipc_send(pkg->id, pkg->type, NULL, 0);
}

static void do_mount_by_index(package_t* pkg) {
	int32_t index = *(int32_t*)get_pkg_data(pkg);
	if(index < 0 || index >= MOUNT_MAX) {
		ipc_send(pkg->id, pkg->type, NULL, 0);
		return;
	}
	mount_t* mnt = &_mounts[index];
	ipc_send(pkg->id, pkg->type, mnt, sizeof(mount_t));
}

static void do_mount_by_fname(package_t* pkg) {
	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	const char* fname = proto_read_str(proto);
	proto_free(proto);

	tree_node_t* node = get_node_by_name(fname);
	if(node == NULL) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}
	int32_t index = FSN(node)->mount;
	if(index < 0 || index >= MOUNT_MAX) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}
	mount_t* mnt = &_mounts[index];
	ipc_send(pkg->id, pkg->type, mnt, sizeof(mount_t));
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
	case VFS_CMD_INFO_UPDATE:
		do_info_update(pkg);
		break;
	case VFS_CMD_NODE_BY_NAME:
		do_node_by_name(pkg);
		break;
	case VFS_CMD_NODE_FULLNAME:
		do_node_fullname(pkg);
		break;
	case VFS_CMD_KID:
		do_kid(pkg);
		break;
	case VFS_CMD_MOUNT:
		do_mount(pkg);
		break;
	case VFS_CMD_UNMOUNT:
		do_unmount(pkg);
		break;
	case VFS_CMD_MOUNT_BY_INDEX:
		do_mount_by_index(pkg);
		break;
	case VFS_CMD_MOUNT_BY_FNAME:
		do_mount_by_fname(pkg);
		break;
	}
}

int main(int argc, char* argv[]) {
  (void)argc;
	(void)argv;

	if(kserv_get_by_name("kserv.vfsd") >= 0) {
    printf("panic: 'kserv.vfsd' process has been running already!\n");
		return -1;
	}

	kserv_register("kserv.vfsd");
	fsnode_init();
	kserv_ready();
	return kserv_run(handle, NULL);
}
