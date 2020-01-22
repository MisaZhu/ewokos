#include <vfs.h>
#include <kstring.h>
#include <mm/kmalloc.h>
#include <kernel/proc.h>
#include <kstring.h>
#include <buffer.h>
#include <proto.h>
#include <kernel/ipc.h>

static vfs_node_t* _vfs_root = NULL;
static mount_t _vfs_mounts[FS_MOUNT_MAX];

static void vfs_node_init(vfs_node_t* node) {
	memset(node, 0, sizeof(vfs_node_t));
	node->fsinfo.node = (uint32_t)node;
	node->fsinfo.mount_id = -1;
}

void vfs_init(void) {
	int32_t i;
	for(i = 0; i<FS_MOUNT_MAX; i++) {
		memset(&_vfs_mounts[i], 0, sizeof(mount_t));
	}

	_vfs_root = vfs_new_node();
	strcpy(_vfs_root->fsinfo.name, "/");
}

static inline int32_t get_mount_pid(vfs_node_t* node) {
	mount_t mount;	
	int32_t res = vfs_get_mount(node, &mount);
	if(res == 0)
		return mount.pid;
	return -1;
}

static inline int32_t check_mount(vfs_node_t* node) {
	if(_current_proc->owner <= 0)
		return 0;

	int32_t mnt_pid = get_mount_pid(node);
	if(mnt_pid != _current_proc->pid) //current proc not the mounting one.
		return -1;
	return 0;
}

static vfs_node_t* vfs_simple_get(vfs_node_t* father, const char* name) {
	if(father == NULL || strchr(name, '/') != NULL)
		return NULL;

	vfs_node_t* node = father->first_kid;
	while(node != NULL) {
		if(strcmp(node->fsinfo.name, name) == 0) {
			return node;
		}
		node = node->next;
	}
	return NULL;
}

int32_t vfs_add(vfs_node_t* father, vfs_node_t* node) {
	if(father == NULL || node == NULL)
		return -1;
	if(check_mount(father) != 0)
		return -1;

	node->father = father;
	if(father->last_kid == NULL) {
		father->first_kid = node;
	}
	else {
		father->last_kid->next = node;
		node->prev = father->last_kid;
	}
	father->kids_num++;
	father->last_kid = node;
	return 0;
}

static int32_t vfs_get_free_mount_id(void) {
	int32_t i;
	for(i = 0; i<FS_MOUNT_MAX; i++) {
		if(_vfs_mounts[i].org_node == 0)
			return i;
	}
	return -1;
}

static void vfs_remove(vfs_node_t* node) {
	if(node == NULL || check_mount(node) != 0)
		return;

	vfs_node_t* father = node->father;
	if(father != NULL) {
		if(father->first_kid == node)
			father->first_kid = node->next;
		if(father->last_kid == node)
			father->last_kid = node->prev;
		father->kids_num--;
	}

	if(node->next != NULL)
		node->next->prev = node->prev;

	if(node->prev != NULL)
		node->prev->next = node->next;

	node->prev = NULL;
	node->next = NULL;
}

static int32_t vfs_get_mount_id(vfs_node_t* node) {
	while(node != NULL) {
		if(node->fsinfo.mount_id >= 0)
			return node->fsinfo.mount_id;
		node = node->father;
	}
	return -1;
}

int32_t vfs_get_mount_by_id(int32_t id, mount_t* mount) {
	if(mount == NULL)
		return -1;

	if(id < 0 || id >= FS_MOUNT_MAX || 
			_vfs_mounts[id].org_node == 0)
		return -1;
	memcpy(mount, &_vfs_mounts[id], sizeof(mount_t));
	return 0;
}

int32_t vfs_get_mount(vfs_node_t* node, mount_t* mount) {
	if(node == NULL || mount == NULL)
		return -1;

	int32_t mount_id = vfs_get_mount_id(node);
	if(mount_id < 0)
		return -1;
	memcpy(mount, &_vfs_mounts[mount_id], sizeof(mount_t));
	return 0;
}

static const char* fullname(vfs_node_t* node) {
	str_t* s1 = str_new("");
	while(node != NULL) {
		str_t* s2 = str_new("");
		str_cpy(s2, node->fsinfo.name);
		if(strlen(CS(s1)) != 0) {
			if(node->fsinfo.name[0] != '/')
				str_addc(s2, '/');
			str_add(s2, CS(s1));
		}
		str_cpy(s1, CS(s2));
		str_free(s2);
		node = node->father;
	}

	static char ret[FS_FULL_NAME_MAX];
	strncpy(ret, CS(s1), FS_FULL_NAME_MAX-1);
	str_free(s1);
	return ret;
}


int32_t vfs_mount(vfs_node_t* org, vfs_node_t* node) {
	if(org == NULL || node == NULL)
		return -1;
		
	if(node->fsinfo.mount_id > 0) //already been mounted 
		return -1;
	
	int32_t id = vfs_get_free_mount_id();
	if(id < 0)
		return -1;

	_vfs_mounts[id].pid = _current_proc->pid;
	_vfs_mounts[id].org_node = (uint32_t)org;
	strcpy(_vfs_mounts[id].org_name, fullname(org));
	strcpy(node->fsinfo.name, org->fsinfo.name);
	node->fsinfo.mount_id =  id;

	vfs_node_t* father = org->father;
	if(father == NULL) {
		_vfs_root = node;	
	}
	else {
		vfs_remove(org);
		vfs_add(father, node);
	}
	return 0;
}

void vfs_umount(vfs_node_t* node) {
	if(node == NULL || node->fsinfo.mount_id < 0 || check_mount(node) != 0)
		return;

	vfs_node_t* org = (vfs_node_t*)_vfs_mounts[node->fsinfo.mount_id].org_node;
	if(org == NULL)
		return;
	
	vfs_node_t* father = node->father;
	if(father == NULL) {
		_vfs_root = org;
	}
	else {
		vfs_remove(node);
		vfs_add(father, org);
	}
}

int32_t vfs_del(vfs_node_t* node) {
	if(node == NULL || node->refs > 0 ||  check_mount(node) != 0)
		return -1;
	/*free children*/
	vfs_node_t* c = node->first_kid;
	while(c != NULL) {
		vfs_node_t* next = c->next;
		vfs_del(c);
		c = next;
	}

	vfs_node_t* father = node->father;
	if(father != NULL) {
		if(father->first_kid == node)
			father->first_kid = node->next;
		if(father->last_kid == node)
			father->last_kid = node->prev;
		father->kids_num--;
	}

	if(node->next != NULL)
		node->next->prev = node->prev;
	if(node->prev != NULL)
		node->prev->next = node->next;
	kfree(node);
	return 0;
}

int32_t vfs_set(vfs_node_t* node, fsinfo_t* info) {
	if(node == NULL ||
			info == NULL || check_mount(node) != 0)
		return -1;
	memcpy(&node->fsinfo, info, sizeof(fsinfo_t));
	return 0;
}

vfs_node_t* vfs_new_node(void) {
	vfs_node_t* ret = (vfs_node_t*)kmalloc(sizeof(vfs_node_t));
	vfs_node_init(ret);
	return ret;
}

vfs_node_t* vfs_get(vfs_node_t* father, const char* name) {
	if(father == NULL)
		return NULL;
	
	if(name[0] == '/') {
		/*go to root*/
		while(father->father != NULL)
			father = father->father;

		name = name+1;
		if(name[0] == 0)
			return father;
	}

	vfs_node_t* node = father;	
	char n[FS_FULL_NAME_MAX+1];
	int32_t j = 0;
	for(int32_t i=0; i<FS_FULL_NAME_MAX; i++) {
		n[i] = name[i];
		if(n[i] == 0) {
			return vfs_simple_get(node, n+j);
		}
		if(n[i] == '/') {
			n[i] = 0; 
			node = vfs_simple_get(node, n+j);
			if(node == NULL)
				return NULL;
			j= i+1;
		}
	}
	return NULL;
}

vfs_node_t* vfs_root(void) {
	return _vfs_root;
}

static int32_t get_free_fd(proc_t* proc) {
	int32_t i;
	for(i=3; i<PROC_FILE_MAX; i++) { //0, 1, 2 reserved for stdio in/out/err
		if(proc->space->files[i].node == 0)
			return i;
	}
	return -1;
}

int32_t vfs_open(int32_t pid, vfs_node_t* node, int32_t wr) {
	if(node == NULL || check_mount(node) != 0)
		return -1;

	proc_t* proc = proc_get(pid);
	if(proc == NULL)
		return -1;

	int32_t fd = get_free_fd(proc);
	if(fd < 0)
		return -1;

	proc->space->files[fd].node = (uint32_t)node;
	proc->space->files[fd].wr = wr;

	node->refs++;
	if(wr != 0)
		node->refs_w++;
	return fd;
}

vfs_node_t* vfs_node_by_fd(int32_t fd) {
	kfile_t* file = &_current_proc->space->files[fd];
	return (vfs_node_t*)file->node;
}

void vfs_close(proc_t* proc, int32_t fd) {
	if(proc == NULL || fd < 0 || fd >= PROC_FILE_MAX)
		return;

	kfile_t* file = &proc->space->files[fd];
	vfs_node_t* node = (vfs_node_t*)file->node;
	if(node == NULL)
		return;

	if(node->refs > 0)
		node->refs--;
	if(file->wr != 0 && node->refs_w > 0)
		node->refs_w--;

	memset(file, 0, sizeof(kfile_t));

	if(node->fsinfo.type == FS_TYPE_PIPE) {
		buffer_t* buffer = (buffer_t*)node->fsinfo.data;
		proc_wakeup((uint32_t)buffer);
		if(node->refs <= 0) {
			kfree(buffer);
			kfree(node);
			return;
		}
	}

	proto_t in;
	proto_init(&in, NULL, 0);
	proto_add_int(&in, FS_CMD_CLOSED);
	proto_add_int(&in, fd);
	proto_add_int(&in, proc->pid);
	proto_add(&in, &node->fsinfo, sizeof(fsinfo_t));

	rawdata_t data;
	data.data = in.data;
	data.size = in.size;

	int32_t to_pid = get_mount_pid(node);
	if(to_pid >= 0) {
		proc_msg_t* msg = proc_send_msg(to_pid, &data, -1);
		if(msg != NULL)
			msg->from_pid = proc->pid;
	}
	proto_clear(&in);
}

int32_t vfs_dup(int32_t from) {
	if(from < 0 || from > PROC_FILE_MAX)
		return -1;
	int32_t to = get_free_fd(_current_proc);
	if(to < 0)
		return -1;

	kfile_t *f = &_current_proc->space->files[from]; 
	vfs_node_t* node = 	(vfs_node_t*)f->node;
	if(node != NULL) {
		memcpy(&_current_proc->space->files[to], f, sizeof(kfile_t));
		node->refs++;
		if(f->wr != 0)
			node->refs_w++;
	}
	return to;
}

int32_t vfs_dup2(int32_t from, int32_t to) {
	if(from == to)
		return to;

	if(from < 0 || from > PROC_FILE_MAX ||
			to < 0 || to > PROC_FILE_MAX)
		return -1;

	vfs_close(_current_proc, to);

	kfile_t *f = &_current_proc->space->files[from]; 
	vfs_node_t* node = 	(vfs_node_t*)f->node;
	if(node != NULL) {
		memcpy(&_current_proc->space->files[to], f, sizeof(kfile_t));
		node->refs++;
		if(f->wr != 0)
			node->refs_w++;
	}
	return to;
}

int32_t vfs_seek(int32_t fd, int32_t offset) {
	if(fd < 0 || fd >= PROC_FILE_MAX)
		return -1;
	
	kfile_t* file = &_current_proc->space->files[fd];
	vfs_node_t* node = (vfs_node_t*)file->node;
	if(node == NULL)
		return -1;

	file->seek = offset;
	return file->seek;
}

int32_t vfs_tell(int32_t fd) {
	if(fd < 0 || fd >= PROC_FILE_MAX)
		return -1;
	
	kfile_t* file = &_current_proc->space->files[fd];
	vfs_node_t* node = (vfs_node_t*)file->node;
	if(node == NULL)
		return -1;

	return file->seek;
}
