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
#include <tstr.h>
#include "pipe.h"

#define DEV_VFS "dev.vfs"
#define MOUNT_MAX 32

static trunk_t _opened;

void opens_init(void) {
	trunk_init(&_opened, 4, malloc, free);
}

static int32_t close_zombie(uint32_t node) {
	fs_info_t info;
	if(node == 0 || syscall2(SYSCALL_PFILE_NODE_BY_ADDR, node, (int32_t)&info) != 0) //all closed
		return 0;
	if(syscall2(SYSCALL_PFILE_GET_REF, (int32_t)node, 2) > 0)
		return -1;

	if(info.dev_serv_pid == getpid())
		do_pipe_close(info.node, true);
	else if(info.dev_serv_pid > 0)
		ipc_req(info.dev_serv_pid, 0, FS_CLOSE, &info, sizeof(fs_info_t), false);
	return 0;
}

static void clear_zombie(void) {
	int32_t i;
	uint32_t* nodes = (uint32_t*)_opened.items;
	for(i=0; i<(int32_t)_opened.size; i++) {
		if(nodes[i] != 0) {
			if(close_zombie(nodes[i]) == 0)
				nodes[i] = 0;
		}	
	}
}

void opens_clear(void) {
	int32_t i;
	uint32_t* nodes = (uint32_t*)_opened.items;
	for(i=0; i<(int32_t)_opened.size; i++) {
		if(nodes[i] != 0) {
			close_zombie(nodes[i]);
			nodes[i] = 0;
		}	
	}
	trunk_clear(&_opened);
}

static int32_t add_opened(uint32_t node) {
	int32_t i;
	uint32_t* nodes = (uint32_t*)_opened.items;
	for(i=0; i<(int32_t)_opened.size; i++) {
		if(nodes[i] == 0) {
			nodes[i] = node;
			return i;
		}	
	}

	i = trunk_add(&_opened);
	nodes = (uint32_t*)_opened.items;
	if(i >= 0)
		nodes[i] = node;
	clear_zombie();
	return i;	
}

static void rm_opened(uint32_t node) {
	int32_t i;
	uint32_t* nodes = (uint32_t*)_opened.items;
	for(i=0; i<(int32_t)_opened.size; i++) {
		if(nodes[i] == node) {
			if(close_zombie(node) == 0)
				nodes[i] = 0;
		}	
	}
}

/**************************/
static mount_t _mounts[MOUNT_MAX];
static tree_node_t* _root = NULL;

static bool check_access(int32_t pid, tree_node_t* node, bool wr) {
	(void)pid;
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
	if(!check_access(pid, node_to, true))
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

static int32_t fsnode_del(int32_t pid, tree_node_t* node) {
	if(!check_access(pid, node, true))
		return -1;
	fs_tree_del(node);
	return 0;
}

static int32_t fsnode_info(int32_t pid, tree_node_t* node, fs_info_t* info) {
	if(!check_access(pid, node, false))
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
		tstr_cpy(FSN(node)->name, "/");
	}
	else
		tstr_cpy(FSN(node)->name, tstr_cstr(FSN(to)->name));

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

static int32_t fsnode_unmount(int32_t pid, tree_node_t* node) {
	if(!check_access(pid, node, true) || node == _root) //can not umount root
		return -1;
	int32_t index = FSN(node)->mount;

	tree_node_t* node_old = (tree_node_t*)_mounts[index].node_old;
	tree_node_t* father = node->father;
	fs_tree_del(node);

	if(node_old != NULL && father != NULL)
		tree_add(father, node_old);
	return 0;
}

static uint32_t get_node_info_by_fd(int32_t pid, int32_t fd) {
	if(fd < 0)
    return 0;
	fs_info_t info;
	if(syscall3(SYSCALL_PFILE_NODE_BY_PID_FD, pid, fd, (int32_t)&info) != 0)
		return 0;
	return info.node;
}

static void do_node_by_fd(package_t* pkg) {
	int32_t fd = *(int32_t*)get_pkg_data(pkg);
	uint32_t node_addr = get_node_info_by_fd(pkg->pid, fd);

	fs_info_t info;
	tree_node_t* node = (tree_node_t*)node_addr;
	if(node == NULL || fsnode_info(pkg->pid, node, &info) != 0) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}
	ipc_send(pkg->id, pkg->type, &info, sizeof(fs_info_t));
}

static void do_add(package_t* pkg) {
	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	uint32_t node = (uint32_t)proto_read_int(proto);
	const char* name = proto_read_str(proto);
	uint32_t size = (uint32_t)proto_read_int(proto);
	void* data = (void*)proto_read_int(proto);
	proto_free(proto);

	if(node == 0) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	tree_node_t* ret = NULL; 
	if(strchr(name, '/') == NULL)
		ret = fsnode_add((tree_node_t*)node, name, size, pkg->pid, data);
	ipc_send(pkg->id, pkg->type, &ret, 4);
}

static void do_del(package_t* pkg) {
	uint32_t node_addr = *(uint32_t*)get_pkg_data(pkg);
	
	tree_node_t* node = (tree_node_t*)node_addr;
	if(node == NULL || fsnode_del(pkg->pid, node) != 0)
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
	else
		ipc_send(pkg->id, pkg->type, NULL, 0);
}

static void do_info(package_t* pkg) {
	uint32_t node = *(uint32_t*)get_pkg_data(pkg);
	fs_info_t info;

	if(node == 0 || fsnode_info(pkg->pid, (tree_node_t*)node, &info) != 0)
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
	if(syscall1(SYSCALL_PFILE_NODE_UPDATE, (int32_t)info) != 0) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}
	ipc_send(pkg->id, pkg->type, NULL, 0);
}

static void do_close(package_t* pkg) {
	int32_t fd = *(int32_t*)get_pkg_data(pkg);
	if(fd < 0)
		return;

	fs_info_t info;
	if(syscall3(SYSCALL_PFILE_NODE_BY_PID_FD, pkg->pid, fd, (int32_t)&info) != 0)
		return;
	if(syscall2(SYSCALL_PFILE_CLOSE, pkg->pid, fd) != 0)
		return;

	if(info.dev_serv_pid == getpid())
		do_pipe_close(info.node, false);
	else if(info.dev_serv_pid > 0)
		ipc_req(info.dev_serv_pid, 0, FS_CLOSE, &info, sizeof(fs_info_t), false);
	rm_opened(info.node);
}

static void do_open(package_t* pkg) {
	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	const char* name = proto_read_str(proto);
	int32_t flags = proto_read_int(proto);
	proto_free(proto);

	fs_info_t info;
	tree_node_t* node = get_node_by_name(name);
	if(node == NULL || fsnode_info(pkg->pid, node, &info) != 0) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}
	
	int32_t fd = syscall3(SYSCALL_PFILE_OPEN, pkg->pid, (int32_t)&info, flags);
	if(fd < 0) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	add_opened((uint32_t)node);
	ipc_send(pkg->id, pkg->type, &fd, 4);
}

static void do_node_by_name(package_t* pkg) {
	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	const char* name = proto_read_str(proto);
	proto_free(proto);

	fs_info_t info;
	tree_node_t* node = get_node_by_name(name);
	if(node == NULL || fsnode_info(pkg->pid, node, &info) != 0) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}
	ipc_send(pkg->id, pkg->type, &info, sizeof(fs_info_t));
}

static void do_node_fullname(package_t* pkg) {
	uint32_t addr = *(uint32_t*)get_pkg_data(pkg);
	tree_node_t* node = (tree_node_t*)addr;

	if(node == NULL) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	tstr_t* full = tstr_new("", malloc, free);
	const char* n;
	while(true) {
		if(node->father == NULL)
			break;
		n = tstr_cstr(FSN(node)->name);
		int32_t j = strlen(n) - 1;
		while(j >= 0) {
			tstr_addc(full, n[j--]);
		}
		tstr_addc(full, '/');
		node = node->father;
	}
	tstr_addc(full, 0);
	tstr_rev(full);
	n = tstr_cstr(full);
	ipc_send(pkg->id, pkg->type, (void*)n, strlen(n)+1);
	tstr_free(full);
}

static void do_node_shortname(package_t* pkg) {
	uint32_t addr = *(uint32_t*)get_pkg_data(pkg);
	tree_node_t* node = (tree_node_t*)addr;
	if(node == NULL) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}
	const char *n = tstr_cstr(FSN(node)->name);
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
	fsnode_info(pkg->pid, n, &info);
	ipc_send(pkg->id, pkg->type, &info, sizeof(fs_info_t));
}

static void do_mount(package_t* pkg) {
	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	const char* fname = proto_read_str(proto);
	const char* dev_name = proto_read_str(proto);
	int32_t dev_index = proto_read_int(proto);
	int32_t isFile = proto_read_int(proto);
	proto_free(proto);

	opens_init();
	tree_node_t* node = fsnode_mount(fname, dev_name, dev_index, isFile, pkg->pid);
	ipc_send(pkg->id, pkg->type, &node, 4);
}

static void do_unmount(package_t* pkg) {
	tree_node_t* node = (tree_node_t*)(*(int32_t*)get_pkg_data(pkg));
	fsnode_unmount(pkg->pid, node);
	opens_clear();
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
	case VFS_CMD_OPEN:
		do_open(pkg);
		break;
	case VFS_CMD_CLOSE:
		do_close(pkg);
		break;
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
	case VFS_CMD_NODE_BY_FD:
		do_node_by_fd(pkg);
		break;
	case VFS_CMD_NODE_FULLNAME:
		do_node_fullname(pkg);
		break;
	case VFS_CMD_NODE_SHORTNAME:
		do_node_shortname(pkg);
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
	case VFS_CMD_PIPE_OPEN:
		do_pipe_open(pkg);
		break;
	case FS_READ:
		do_pipe_read(pkg);
		break;
	case FS_WRITE:
		do_pipe_write(pkg);
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
