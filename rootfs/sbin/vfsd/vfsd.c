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
static mount_t _mounts[MOUNT_MAX];
static tree_node_t* _root = NULL;
static trunk_t _opened;

typedef struct {
	int32_t pid;
	int32_t fd;
	uint32_t node;
} opened_t;

static void fsnode_init(void) {
	for(int i=0; i<MOUNT_MAX; i++) {
		memset(&_mounts[i], 0, sizeof(mount_t));
	}
	//strcpy(_mounts[0].dev_name, DEV_VFS);
	_root = NULL;
}

void opens_init(void) {
	trunk_init(&_opened, sizeof(opened_t), MFS);
}

static bool check_access(int32_t pid, tree_node_t* node, bool wr) {
	(void)pid;
	(void)wr;
	if(node == NULL)
		return false;
	return true;
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

static int32_t close_zombie(opened_t* opened) {
	fs_info_t info;
	if(opened == NULL || opened->node == 0)
		return 0;
	if(syscall2(SYSCALL_PFILE_GET_REF, (int32_t)opened->node, 2) > 0)
		return -1;
	fsnode_info(-1, (tree_node_t*)opened->node, &info);
	if(info.dev_serv_pid == getpid())
		do_pipe_close(info.node, true);
	else if(info.dev_serv_pid > 0) {
		proto_t* proto = proto_new(NULL, 0);
		proto_add_int(proto, opened->pid);
		proto_add_int(proto, opened->fd);
		proto_add(proto, &info, sizeof(fs_info_t));
		ipc_call(info.dev_serv_pid, FS_CLOSE, proto, NULL);
		proto_free(proto);
	}		
	return 0;
}

static void clear_zombie(void) {
	int32_t i;
	opened_t* opened = (opened_t*)_opened.items;
	for(i=0; i<(int32_t)_opened.size; i++) {
		if(opened[i].node != 0 && close_zombie(&opened[i]) == 0)
			opened[i].node = 0;
	}
}

void opens_clear(void) {
	int32_t i;
	opened_t* opened = (opened_t*)_opened.items;
	for(i=0; i<(int32_t)_opened.size; i++) {
		if(opened[i].node != 0) {
			close_zombie(&opened[i]);
			opened[i].node = 0;
		}	
	}
	trunk_clear(&_opened);
}

static int32_t add_opened(int32_t pid, int32_t fd, uint32_t node) {
	int32_t i;
	opened_t* opened = (opened_t*)_opened.items;
	for(i=0; i<(int32_t)_opened.size; i++) {
		if(opened[i].node != 0) {
			opened[i].pid = pid;
			opened[i].fd = fd;
			opened[i].node = node;
			return i;
		}	
	}

	i = trunk_add(&_opened);
	opened = (opened_t*)_opened.items;
	if(i >= 0) {
		opened[i].pid = pid;
		opened[i].fd = fd;
		opened[i].node = node;
	}
	clear_zombie();
	return i;	
}

static void rm_opened(uint32_t node) {
	int32_t i;
	opened_t* opened = (opened_t*)_opened.items;
	for(i=0; i<(int32_t)_opened.size; i++) {
		if(opened[i].node == node) {
			if(syscall2(SYSCALL_PFILE_GET_REF, (int32_t)node, 2) == 0)
				opened[i].node = 0;
		}	
	}
}

/**************************/

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

static tree_node_t* get_node_by_name(const char* fname) {
	return fs_tree_get(_root, fname);
}

static int32_t get_info_by_name(int32_t pid, const char* fname, fs_info_t* info) {
	tree_node_t* node = get_node_by_name(fname);
	if(node == NULL)
		return -1;
	return fsnode_info(pid, node, info);
}

static tree_node_t* get_node_by_fd(int32_t pid, int32_t fd) {
	if(fd < 0)
    return NULL;
	fs_info_t info;
	if(syscall3(SYSCALL_PFILE_INFO_BY_PID_FD, pid, fd, (int32_t)&info) != 0)
		return NULL;
	return (tree_node_t*)info.node;
}

static int32_t get_info_by_fd(int32_t pid, int32_t fd, fs_info_t* info) {
	return syscall3(SYSCALL_PFILE_INFO_BY_PID_FD, pid, fd, (int32_t)info);
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
		_mounts[i].node_old = (uint32_t)_root;
	}
	else
		tstr_cpy(FSN(node)->name, CS(FSN(to)->name));

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

static int32_t do_info_by_fd(int32_t pid, proto_t* in, proto_t* out) {
	int32_t fd = proto_read_int(in);
	tree_node_t* node = get_node_by_fd(pid, fd);
	fs_info_t info;
	if(node == NULL || fsnode_info(pid, node, &info) != 0) {
		return -1;
	}
	proto_add(out, &info, sizeof(fs_info_t));
	return 0;
}

static int32_t do_add(int32_t pid, proto_t* in) {
	const char* dir_name = proto_read_str(in);
	const char* name = proto_read_str(in);
	uint32_t size = (uint32_t)proto_read_int(in);
	void* data = (void*)proto_read_int(in);

	fs_info_t info;
	if(get_info_by_name(pid, dir_name, &info) != 0) {
		return -1;
	}

	tree_node_t* ret = NULL; 
	if(strchr(name, '/') == NULL)
		ret = fsnode_add((tree_node_t*)info.node, name, size, pid, data);
	if(ret == NULL)
		return -1;
	return 0;
}

static int32_t do_del(int32_t pid, proto_t* in) {
	const char* fname = proto_read_str(in);
	fs_info_t info;
	if(get_info_by_name(pid, fname, &info) != 0) {
		return -1;
	}

	if(info.dev_serv_pid > 0) {
		proto_t* proto = proto_new(NULL, 0);
		proto_add_str(proto, fname);
		proto_add(proto, &info, sizeof(fs_info_t));
		int32_t res = ipc_call(info.dev_serv_pid, FS_REMOVE, proto, NULL);
		proto_free(proto);
		if(res != 0) {
			return -1;
		}
	}

	if(fsnode_del(pid, (tree_node_t*)info.node) != 0)
		return -1;
	return 0;
}

static int32_t do_info_update(int32_t pid, proto_t* in) {
	fs_info_t* info = proto_read(in, NULL);
	if(info == NULL || info->node == 0) {
		return -1;
	}

	tree_node_t* node = (tree_node_t*)info->node;
	if(pid != _mounts[FSN(node)->mount].dev_serv_pid) {
		return -1;
	}

	FSN(node)->size = info->size;
	FSN(node)->data = info->data;
	FSN(node)->owner = info->owner;
	return 0;
}

static int32_t do_close(int pid, proto_t* in) {
	int32_t fd = proto_read_int(in);
	if(fd < 0) {
		return -1;
	}

	fs_info_t info;
	if(get_info_by_fd(pid, fd, &info) != 0) {
		return -1;
	}
	syscall2(SYSCALL_PFILE_CLOSE, pid, fd);

	tree_node_t* node = (tree_node_t*)info.node;
	FSN(node)->size = info.size;
	if(info.data != NULL)
		FSN(node)->data = info.data;

	if(info.dev_serv_pid == getpid())
		do_pipe_close(info.node, false);
	else if(info.dev_serv_pid > 0) {
		proto_t* proto = proto_new(NULL, 0);
		proto_add_int(proto, pid);
		proto_add_int(proto, fd);
		proto_add(proto, &info, sizeof(fs_info_t));
		ipc_call(info.dev_serv_pid, FS_CLOSE, proto, NULL);
		proto_free(proto);
	}		
	rm_opened(info.node);
	return 0;
}

static int32_t do_open(int32_t pid, proto_t* in, proto_t* out) {
	const char* name = proto_read_str(in);
	int32_t flags = proto_read_int(in);
	proto_free(in);

	fs_info_t info;
	if(get_info_by_name(pid, name, &info) != 0) {
		return -1;
	}
	
	int32_t fd = syscall3(SYSCALL_PFILE_OPEN, pid, (int32_t)&info, flags);
	if(fd < 0) {
		return -1;
	}

	add_opened(pid, fd, info.node);
	proto_add_int(out, fd);
	return 0;
}

static int32_t do_info_by_name(int32_t pid, proto_t* in, proto_t* out) {
	const char* name = proto_read_str(in);
	fs_info_t info;
	if(get_info_by_name(pid, name, &info) != 0) {
		return -1;
	}
	proto_add(out, &info, sizeof(fs_info_t));
	return 0;
}

static tstr_t* get_node_fullname(tree_node_t* node) {
	tstr_t* full = tstr_new("", MFS);
	const char* n;
	while(true) {
		if(node->father == NULL)
			break;
		n = CS(FSN(node)->name);
		int32_t j = strlen(n) - 1;
		while(j >= 0) {
			tstr_addc(full, n[j--]);
		}
		tstr_addc(full, '/');
		node = node->father;
	}
	tstr_addc(full, 0);
	tstr_rev(full);

	if(full->size == 0) 
		tstr_cpy(full, "/");
	return full;
}

static int32_t do_fullname_by_fd(int32_t pid, proto_t* in, proto_t* out) {
	int32_t fd = proto_read_int(in);
	tree_node_t* node = get_node_by_fd(pid, fd);
	if(node == NULL) {
		return -1;
	}
	tstr_t* full = get_node_fullname(node); 
	proto_add_str(out, CS(full));
	tstr_free(full);
	return 0;
}

static int32_t do_fullname_by_node(proto_t* in, proto_t* out) {
	uint32_t node_addr = (uint32_t)proto_read_int(in);
	tree_node_t* node = (tree_node_t*)node_addr;
	if(node == NULL) {
		return -1;
	}
	tstr_t* full = get_node_fullname(node); 
	proto_add_str(out, CS(full));
	tstr_free(full);
	return 0;
}

static int32_t do_shortname_by_fd(int32_t pid, proto_t* in, proto_t* out) {
	int32_t fd = proto_read_int(in);
	tree_node_t* node = get_node_by_fd(pid, fd);
	if(node == NULL) {
		return -1;
	}
	const char *n = CS(FSN(node)->name);
	proto_add_str(out, n);
	return 0;
}

static int32_t do_shortname_by_node(proto_t* in, proto_t* out) {
	uint32_t node_addr = (uint32_t)proto_read_int(in);
	tree_node_t* node = (tree_node_t*)node_addr;
	if(node == NULL) {
		return -1;
	}
	const char *n = CS(FSN(node)->name);
	proto_add_str(out, n);
	return 0;
}

static int32_t do_kid(proto_t* in, proto_t* out) {
	const char* dir_name = proto_read_str(in);
	int32_t index = proto_read_int(in);

	tree_node_t* node = get_node_by_name(dir_name);
	if(node == NULL || index < 0 || index >= (int32_t)node->size) {
		return -1;
	}

	int32_t i;
	tree_node_t* n = node->fChild;
	for(i=0; i<index; i++) {
		if(n == NULL)
			break;
		n = n->next;
	}
	if(n == NULL) {
		return -1;
	}
	const char *name = CS(FSN(n)->name);
	proto_add_str(out, name);
	return 0;
}

static int32_t do_mount(int32_t pid, proto_t* in) {
	const char* fname = proto_read_str(in);
	const char* dev_name = proto_read_str(in);
	int32_t dev_index = proto_read_int(in);
	int32_t isFile = proto_read_int(in);

	tree_node_t* node = fsnode_mount(fname, dev_name, dev_index, isFile, pid);
	if(node == NULL)
		return -1;
	return 0;
}

static int32_t do_unmount(int32_t pid, proto_t* in) {
	fs_info_t info;
	const char* fname = proto_read_str(in);
	if(get_info_by_name(pid, fname, &info) != 0) {
		return -1;
	}

	fsnode_unmount(pid, (tree_node_t*)info.node);
	return 0;
}

static int32_t do_mount_by_index(proto_t* in, proto_t* out) {
	int32_t index = proto_read_int(in);
	if(index < 0 || index >= MOUNT_MAX) {
		return -1;
	}
	mount_t* mnt = &_mounts[index];
	proto_add(out, mnt, sizeof(mount_t));
	return 0;
}

static int32_t do_mount_by_fname(proto_t* in, proto_t* out) {
	const char* fname = proto_read_str(in);
	tree_node_t* node = get_node_by_name(fname);
	if(node == NULL) {
		return -1;
	}
	int32_t index = FSN(node)->mount;
	if(index < 0 || index >= MOUNT_MAX) {
		return -1;
	}
	mount_t* mnt = &_mounts[index];
	proto_add(out, mnt, sizeof(mount_t));
	return 0;
}

static int32_t ipccall(int32_t pid, int32_t call_id, proto_t* in, proto_t* out, void* p) {
	(void)p;
	switch(call_id) {
	case VFS_CMD_OPEN:
		return do_open(pid, in, out);
	case VFS_CMD_CLOSE:
		return do_close(pid, in);
	case VFS_CMD_ADD:
		return do_add(pid, in);
	case VFS_CMD_DEL:
		return do_del(pid, in);
	case VFS_CMD_INFO_UPDATE:
		return do_info_update(pid, in);
	case VFS_CMD_INFO_BY_NAME:
		return do_info_by_name(pid, in, out);
	case VFS_CMD_INFO_BY_FD:
		return do_info_by_fd(pid, in, out);
	case VFS_CMD_FULLNAME_BY_FD:
		return do_fullname_by_fd(pid, in, out);
	case VFS_CMD_SHORTNAME_BY_FD:
		return do_shortname_by_fd(pid, in, out);
	case VFS_CMD_FULLNAME_BY_NODE:
		return do_fullname_by_node(in, out);
	case VFS_CMD_SHORTNAME_BY_NODE:
		return do_shortname_by_node(in, out);
	case VFS_CMD_KID:
		return do_kid(in, out);
	case VFS_CMD_MOUNT:
		return do_mount(pid, in);
	case VFS_CMD_UNMOUNT:
		return do_unmount(pid, in);
	case VFS_CMD_MOUNT_BY_INDEX:
		return do_mount_by_index(in, out);
	case VFS_CMD_MOUNT_BY_FNAME:
		return do_mount_by_fname(in, out);
	case VFS_CMD_PIPE_OPEN:
		return do_pipe_open(pid, out);
	case FS_READ:
		return do_pipe_read(pid, in, out);
	case FS_WRITE:
		return do_pipe_write(pid, in, out);
	}
	return -1;
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
	opens_init();

	kserv_ready();
	int ret = kserv_run(ipccall, NULL, NULL, NULL);

	opens_clear();
	return ret;
}
