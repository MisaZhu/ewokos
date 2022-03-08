#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/klog.h>
#include <sys/proc.h>
#include <sys/mstr.h>
#include <sys/buffer.h>
#include <sys/proto.h>
#include <sys/fsinfo.h>
#include <sys/vfsc.h>
#include <sys/syscall.h>
#include <procinfo.h>

#define PROC_FILE_MAX 128

typedef struct vfs_node {
  struct vfs_node* father;
  struct vfs_node* first_kid; /*first child*/
  struct vfs_node* last_kid; /*last child*/
  struct vfs_node* next; /*next brother*/
  struct vfs_node* prev; /*prev brother*/
  uint32_t kids_num;

  fsinfo_t fsinfo;

	int32_t mount_id;
  uint32_t refs;
  uint32_t refs_w;
} vfs_node_t;

typedef struct {
  vfs_node_t *node;
  uint32_t flags;
  uint32_t seek;
} file_t;

static vfs_node_t* _vfs_root = NULL;
static mount_t _vfs_mounts[FS_MOUNT_MAX];

typedef struct {
	file_t fds[PROC_FILE_MAX];
} proc_fds_t;

static proc_fds_t _proc_fds_table[PROC_MAX];

static void vfs_node_init(vfs_node_t* node) {
	memset(node, 0, sizeof(vfs_node_t));
	node->fsinfo.node = (uint32_t)node;
	node->mount_id = -1;
}

static vfs_node_t* vfs_new_node(void) {
	vfs_node_t* ret = (vfs_node_t*)malloc(sizeof(vfs_node_t));
	vfs_node_init(ret);
	return ret;
}

static void vfs_init(void) {
	int32_t i;
	for(i = 0; i<FS_MOUNT_MAX; i++) {
		memset(&_vfs_mounts[i], 0, sizeof(mount_t));
	}

	for(i = 0; i<PROC_MAX; i++) {
		for(int j = 0; j<PROC_FILE_MAX; j++)
			memset(&_proc_fds_table[i].fds[j], 0, sizeof(file_t));
	}

	_vfs_root = vfs_new_node();
	strcpy(_vfs_root->fsinfo.name, "/");
}

static file_t* vfs_get_file(int32_t pid, int32_t fd) {
	if(pid < 0 || pid >= PROC_MAX || fd < 0 || fd >= PROC_FILE_MAX)
		return NULL;
	return &_proc_fds_table[pid].fds[fd];
}

static int32_t vfs_get_mount_id(vfs_node_t* node) {
	while(node != NULL) {
		if(node->mount_id >= 0)
			return node->mount_id;
		node = node->father;
	}
	return -1;
}

static int32_t vfs_get_mount(vfs_node_t* node, mount_t* mount) {
	if(node == NULL || mount == NULL)
		return -1;

	int32_t mount_id = vfs_get_mount_id(node);
	if(mount_id < 0)
		return -1;
	memcpy(mount, &_vfs_mounts[mount_id], sizeof(mount_t));
	return 0;
}

static inline int32_t get_mount_pid(vfs_node_t* node) {
	mount_t mount;	
	int32_t res = vfs_get_mount(node, &mount);
	if(res == 0)
		return mount.pid;
	return -1;
}

static inline int32_t check_mount(int32_t pid, vfs_node_t* node) {
	(void)pid;
	(void)node;
	/*int32_t owner = syscall0(SYS_PROC_GET_UID);
	if(owner <= 0)
		return 0;

	int32_t mnt_pid = get_mount_pid(node);
	if(mnt_pid != pid) //current proc not the mounting one.
		return -1;
		*/
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

static vfs_node_t* vfs_get_by_name(vfs_node_t* father, const char* name) {
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

static int32_t vfs_add(int32_t pid, vfs_node_t* father, vfs_node_t* node) {
	if(father == NULL || node == NULL)
		return -1;
	if(check_mount(pid, father) != 0)
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

static void vfs_remove(int32_t pid, vfs_node_t* node) {
	if(node == NULL || check_mount(pid, node) != 0)
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

static int32_t vfs_get_mount_by_id(int32_t id, mount_t* mount) {
	if(mount == NULL)
		return -1;

	if(id < 0 || id >= FS_MOUNT_MAX || 
			_vfs_mounts[id].org_node == 0)
		return -1;
	memcpy(mount, &_vfs_mounts[id], sizeof(mount_t));
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

static int32_t vfs_mount(int32_t pid, vfs_node_t* org, vfs_node_t* node) {
	if(org == NULL || node == NULL)
		return -1;
		
	if(node->mount_id >= 0) //already been mounted 
		return -1;
	
	int32_t id = vfs_get_free_mount_id();
	if(id < 0)
		return -1;

	_vfs_mounts[id].pid = pid;
	_vfs_mounts[id].org_node = (uint32_t)org;
	strcpy(_vfs_mounts[id].org_name, fullname(org));
	strcpy(node->fsinfo.name, org->fsinfo.name);
	node->mount_id =  id;

	vfs_node_t* father = org->father;
	if(father == NULL) {
		_vfs_root = node;	
	}
	else {
		vfs_remove(pid, org);
		vfs_add(pid, father, node);
	}
	return 0;
}

static void vfs_umount(int32_t pid, vfs_node_t* node) {
	if(node == NULL || node->mount_id < 0 || check_mount(pid, node) != 0)
		return;

	vfs_node_t* org = (vfs_node_t*)_vfs_mounts[node->mount_id].org_node;
	if(org == NULL) {
		return;
	}
	
	vfs_node_t* father = node->father;
	if(father == NULL) {
		_vfs_root = org;
	}
	else {
		vfs_remove(pid, node);
		if(org->mount_id < 0)
			free(org);
		else
			vfs_add(pid, father, org);
	}
	memset(&_vfs_mounts[node->mount_id], 0, sizeof(mount_t));
}

static int32_t vfs_del(int32_t pid, vfs_node_t* node) {
	if(node == NULL || node->refs > 0 ||  check_mount(pid, node) != 0)
		return -1;
	/*free children*/
	vfs_node_t* c = node->first_kid;
	while(c != NULL) {
		vfs_node_t* next = c->next;
		vfs_del(pid, c);
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
	free(node);
	return 0;
}

static int32_t vfs_set(int32_t pid, vfs_node_t* node, fsinfo_t* info) {
	if(node == NULL ||
			info == NULL || check_mount(pid, node) != 0)
		return -1;
	memcpy(&node->fsinfo, info, sizeof(fsinfo_t));
	return 0;
}

vfs_node_t* vfs_root(void) {
	return _vfs_root;
}

static int32_t get_free_fd(int32_t pid) {
	int32_t i;
	for(i=3; i<PROC_FILE_MAX; i++) { //0, 1, 2 reserved for stdio in/out/err
		if(_proc_fds_table[pid].fds[i].node == 0)
			return i;
	}
	return -1;
}

static int32_t vfs_open(int32_t pid, vfs_node_t* node, int32_t flags) {
	if(node == NULL || check_mount(pid, node) != 0)
		return -1;

	int32_t fd = get_free_fd(pid);
	if(fd < 0)
		return -1;

	_proc_fds_table[pid].fds[fd].node = node;
	_proc_fds_table[pid].fds[fd].flags = flags;

	node->refs++;
	if((flags & O_WRONLY) != 0)
		node->refs_w++;
	return fd;
}

static vfs_node_t* vfs_get_by_fd(int32_t pid, int32_t fd) {
	file_t* file = vfs_get_file(pid, fd);
	if(file == NULL)
		return NULL;

	return file->node;
}

typedef struct st_close_event {
	int fd;
	int owner_pid;
	int dev_pid;
	fsinfo_t info;

	struct st_close_event* next;
} close_event_t;

static close_event_t* _event_head = NULL;
static close_event_t* _event_tail = NULL;

static void push_close_event(close_event_t* ev) {
  close_event_t* e = (close_event_t*)malloc(sizeof(close_event_t));
  memcpy(e, ev, sizeof(close_event_t));
  e->next = NULL;

  if(_event_tail != NULL)
    _event_tail->next = e;
  else
    _event_head = e;
  _event_tail = e;
	proc_wakeup((uint32_t)_vfs_root);
}

static int get_close_event(close_event_t* ev) {
  close_event_t* e = _event_head;
  if(e == NULL) {
		proc_block(getpid(), (uint32_t)_vfs_root);
    return -1;
	}
	ipc_lock();
  _event_head = _event_head->next;
  if(_event_head == NULL)
    _event_tail = NULL;

  memcpy(ev, e, sizeof(close_event_t));
  free(e);
	ipc_unlock();
  return 0;
}

static void proc_file_close(int pid, int fd, file_t* file, bool close_dev) {
	(void)close_dev;
	(void)pid;
	(void)fd;
	if(file == NULL)
		return;

	vfs_node_t* node = (vfs_node_t*)file->node;
	if(node == NULL)
		return;

	if(node->refs > 0)
		node->refs--;
	if((file->flags & O_WRONLY) != 0 && node->refs_w > 0)
		node->refs_w--;

	if(node->fsinfo.type == FS_TYPE_PIPE) {
		proc_wakeup((int32_t)node);
		if(node->refs <= 0) {
			buffer_t* buffer = (buffer_t*)node->fsinfo.data;
			if(buffer != NULL)
				free(buffer);
			free(node);
			file->node = 0;
		}
	}

	if(!close_dev)
		return;

	int32_t to_pid = get_mount_pid(node);
	if(to_pid < 0)
		return;

	close_event_t ev;
	ev.fd = fd;
	ev.owner_pid = pid;
	ev.dev_pid = to_pid;
	memcpy(&ev.info, &node->fsinfo, sizeof(fsinfo_t));
	push_close_event(&ev);

	/*proto_t in;
	PF->init(&in)->
			addi(&in, fd)->
			addi(&in, pid)->
			add(&in, &node->fsinfo, sizeof(fsinfo_t));
	ipc_call(to_pid, FS_CMD_CLOSE, &in, NULL);
	PF->clear(&in);
	*/
}

static void vfs_close(int32_t pid, int32_t fd) {
	if(pid < 0 || fd < 0 || fd >= PROC_FILE_MAX)
		return;

	file_t* f = vfs_get_file(pid, fd);
	proc_file_close(pid, fd, f, false);
	memset(f, 0, sizeof(file_t));
}

static int32_t vfs_dup(int32_t pid, int32_t from) {
	if(from < 0 || from > PROC_FILE_MAX)
		return -1;
	int32_t to = get_free_fd(pid);
	if(to < 0)
		return -1;

	file_t* f = vfs_get_file(pid, from);
	if(f->node == NULL) 
		return -1;

	file_t* f_to = vfs_get_file(pid, to);
	memcpy(f_to, f, sizeof(file_t));
	f->node->refs++;
	if((f->flags & O_WRONLY) != 0)
		f->node->refs_w++;
	return to;
}

static int32_t vfs_dup2(int32_t pid, int32_t from, int32_t to) {
	if(from == to)
		return to;

	if(from < 0 || from > PROC_FILE_MAX ||
			to < 0 || to > PROC_FILE_MAX)
		return -1;

	vfs_close(pid, to);

	file_t* f = vfs_get_file(pid, from);
	if(f->node == NULL) 
		return -1;

	file_t* f_to = vfs_get_file(pid, to);
	memcpy(f_to, f, sizeof(file_t));
	f->node->refs++;
	if((f->flags & O_WRONLY) != 0)
		f->node->refs_w++;
	return to;
}

static inline fsinfo_t* gen_fsinfo(vfs_node_t* node) {
	static fsinfo_t ret;
	memcpy(&ret, &node->fsinfo, sizeof(fsinfo_t));
	ret.mount_pid = get_mount_pid(node);
	return &ret;
}

static int32_t vfs_seek(int32_t pid, int32_t fd, int32_t offset) {
	if(fd < 0 || fd >= PROC_FILE_MAX)
		return -1;
	
	file_t* f = vfs_get_file(pid, fd);
	if(f->node == NULL)
		return -1;

	f->seek = offset;
	return f->seek;
}

static int32_t vfs_tell(int32_t pid, int32_t fd) {
	if(fd < 0 || fd >= PROC_FILE_MAX)
		return -1;
	
	file_t* f = vfs_get_file(pid, fd);
	if(f->node == NULL)
		return -1;

	return f->seek;
}

static fsinfo_t* vfs_get_kids(vfs_node_t* father, uint32_t* num) {
	*num = 0;
	if(father == NULL || father->kids_num == 0)
		return NULL;
	
	uint32_t n = father->kids_num;
	fsinfo_t* ret = (fsinfo_t*)malloc(n * sizeof(fsinfo_t));
	if(ret == NULL) {
		*num = 0;
		return NULL;
	}

	uint32_t i = 0;
	vfs_node_t* node = father->first_kid;
	while(node != NULL && i<n) {
		memcpy(&ret[i], gen_fsinfo(node), sizeof(fsinfo_t));
		node = node->next;
		i++;
	}

	*num = i;
	return ret;
}

/*---------------------------------------*/

static void do_vfs_get_by_name(proto_t* in, proto_t* out) {
	const char* name = proto_read_str(in);
  vfs_node_t* node = vfs_get_by_name(vfs_root(), name);
  if(node == NULL) {
		PF->addi(out, 0);
    return;
	}
	PF->addi(out, (int32_t)node)->add(out, gen_fsinfo(node), sizeof(fsinfo_t));
}

static void do_vfs_get_by_fd(int pid, proto_t* in, proto_t* out) {
	int fd = proto_read_int(in);
  vfs_node_t* node = vfs_get_by_fd(pid, fd);
  if(node == NULL) {
		PF->addi(out, 0);
    return;
	}
	PF->add(out, gen_fsinfo(node), sizeof(fsinfo_t));
}

static void do_vfs_get_flags(int pid, proto_t* in, proto_t* out) {
	int fd = proto_read_int(in);
	file_t* file = vfs_get_file(pid, fd);
  if(file == NULL) {
		PF->addi(out, -1);
    return;
	}
	PF->addi(out, 0)->addi(out, file->flags);
}

static void do_vfs_set_flags(int pid, proto_t* in, proto_t* out) {
	int fd = proto_read_int(in);
	int flags = proto_read_int(in);
	file_t* file = vfs_get_file(pid, fd);
  if(file == NULL) {
		PF->addi(out, -1);
    return;
	}
	file->flags = flags;
	PF->addi(out, 0);
}

static void do_vfs_new_node(proto_t* in, proto_t* out) {
	fsinfo_t info;
	if(proto_read_to(in, &info, sizeof(fsinfo_t)) != sizeof(fsinfo_t))
		return;
 	vfs_node_t* node = vfs_new_node();
 	if(node == NULL)
		return;
	info.node = (uint32_t)node;
	info.mount_pid = -1;
	memcpy(&node->fsinfo, &info, sizeof(fsinfo_t));
	PF->add(out, &info, sizeof(fsinfo_t));
}

static void do_vfs_open(int32_t pid, proto_t* in, proto_t* out) {
	fsinfo_t info;
	PF->addi(out, -1);

	if(proto_read_to(in, &info, sizeof(fsinfo_t)) != sizeof(fsinfo_t))
		return;
	int32_t flags = proto_read_int(in);
 	vfs_node_t* node = (vfs_node_t*)info.node;
 	if(node == NULL)
		return;

	int res = vfs_open(pid, node, flags);
	PF->clear(out);
	PF->addi(out, res);
}

static void do_vfs_close(int32_t pid, proto_t* in) {
	int fd = proto_read_int(in);
	if(fd < 0)
		return;

	vfs_close(pid, fd);
}

static void do_vfs_tell(int32_t pid, proto_t* in, proto_t* out) {
	int fd = proto_read_int(in);
	PF->addi(out, vfs_tell(pid, fd));
}

static void do_vfs_seek(int32_t pid, proto_t* in, proto_t* out) {
	int fd = proto_read_int(in);
	int seek = proto_read_int(in);
	PF->addi(out, vfs_seek(pid, fd, seek));
}

static void do_vfs_dup(int32_t pid, proto_t* in, proto_t* out) {
	int fd = proto_read_int(in);
	PF->addi(out, vfs_dup(pid, fd));
}

static void do_vfs_dup2(int32_t pid, proto_t* in, proto_t* out) {
	int fd = proto_read_int(in);
	int fdto = proto_read_int(in);
	PF->addi(out, vfs_dup2(pid, fd, fdto));
}

static void do_vfs_set_fsinfo(int32_t pid, proto_t* in, proto_t* out) {
	fsinfo_t info;
	int32_t res = -1;
	if(proto_read_to(in, &info, sizeof(fsinfo_t)) == sizeof(fsinfo_t)) {
  	vfs_node_t* node = (vfs_node_t*)info.node;
  	if(node != NULL) { 
			vfs_set(pid, node, &info);
			res = 0;
		}
	}
	PF->addi(out, res);
}

static void do_vfs_get_kids(proto_t* in, proto_t* out) {
	fsinfo_t info;
	PF->addi(out, 0);
	if(proto_read_to(in, &info, sizeof(fsinfo_t)) != sizeof(fsinfo_t))
		return;

  vfs_node_t* node = (vfs_node_t*)info.node;
  if(node == NULL)
    return;
	
	uint32_t num = 0;
	fsinfo_t* kids = vfs_get_kids(node, &num);
	if(kids == NULL || num == 0)
		return;
	PF->clear(out)->addi(out, num)->add(out, kids, sizeof(fsinfo_t)*num);
}

static void do_vfs_add(int32_t pid, proto_t* in, proto_t* out) {
	fsinfo_t info_to;
	fsinfo_t info;

	PF->addi(out, -1);
	if(proto_read_to(in, &info_to, sizeof(fsinfo_t)) != sizeof(fsinfo_t))
		return;
	if(proto_read_to(in, &info, sizeof(fsinfo_t)) != sizeof(fsinfo_t))
		return;

  vfs_node_t* node_to = (vfs_node_t*)info_to.node;
	if(node_to == NULL) 
    return;

  vfs_node_t* node = vfs_get_by_name(node_to, info.name);
  if(node != NULL)
    return;

  node = (vfs_node_t*)info.node;
  if(node == NULL)
    return;

	vfs_add(pid, node_to, node);
	PF->clear(out)->addi(out, 0);
}

static void do_vfs_del(int32_t pid, proto_t* in, proto_t* out) {
	fsinfo_t info;

	PF->addi(out, -1);
	if(proto_read_to(in, &info, sizeof(fsinfo_t)) != sizeof(fsinfo_t))
		return;

  vfs_node_t* node = (vfs_node_t*)info.node;
  if(node == NULL)
    return;

	int res = vfs_del(pid, node);
	PF->clear(out)->addi(out, res);
}

static void do_vfs_mount(int32_t pid, proto_t* in, proto_t* out) {
	fsinfo_t info_to;
	fsinfo_t info;

	PF->addi(out, -1);
	if(proto_read_to(in, &info_to, sizeof(fsinfo_t)) != sizeof(fsinfo_t))
		return;
	if(proto_read_to(in, &info, sizeof(fsinfo_t)) != sizeof(fsinfo_t))
		return;

  vfs_node_t* node_to = (vfs_node_t*)info_to.node;
  vfs_node_t* node = (vfs_node_t*)info.node;
	if(node_to == NULL || node == NULL)
    return;

	vfs_mount(pid, node_to, node);
	PF->clear(out)->addi(out, 0);
}

static void do_vfs_umount(int32_t pid, proto_t* in) {
	fsinfo_t info;
	if(proto_read_to(in, &info, sizeof(fsinfo_t)) != sizeof(fsinfo_t))
		return;

  vfs_node_t* node = (vfs_node_t*)info.node;
	if(node == NULL)
    return;

	vfs_umount(pid, node);
}

static void do_vfs_get_mount_by_id(proto_t* in, proto_t* out) {
	mount_t mount;
	if(vfs_get_mount_by_id(proto_read_int(in), &mount) != 0) {
		PF->addi(out, -1);
		return;
	}
	PF->addi(out, 0)->add(out, &mount, sizeof(mount_t));
}

static void do_vfs_pipe_open(int32_t pid, proto_t* out) {
  vfs_node_t* node = vfs_new_node();
	PF->addi(out, -1);
  if(node == NULL) {
    return;
	}
  node->fsinfo.type = FS_TYPE_PIPE;
  node->fsinfo.data = 0; //buffer set as zero , waiting for other-port

  int32_t fd0 = vfs_open(pid, node, 1);
  if(fd0 < 0) {
    free(node);
		return;
  }

  int32_t fd1 = vfs_open(pid, node, 1);
  if(fd1 < 0) {
    vfs_close(pid, fd0);
    free(node);
		return;
  }
	PF->clear(out)->addi(out, 0)->addi(out, fd0)->addi(out, fd1);
}

static void do_vfs_pipe_write(int pid, proto_t* in, proto_t* out) {
	(void)pid;
	fsinfo_t info;
	PF->addi(out, -1);
	if(proto_read_to(in, &info, sizeof(fsinfo_t)) != sizeof(fsinfo_t)) {
		return;
	}

  vfs_node_t* node = (vfs_node_t*)info.node;
	proc_wakeup((int32_t)node); //wakeup reader

	int32_t size = 0;
	void *data = proto_read(in, &size);
	if(data == NULL) {
		return;
	}

	buffer_t* buffer = (buffer_t*)info.data;
	if(buffer == NULL) {
		PF->clear(out)->addi(out, 0); // pipe still waiting for other-port, retry
		return;
	}

	size = buffer_write(buffer, data, size);
	if(size > 0) {
		PF->clear(out)->addi(out, size);
		return;
	}

	if(node == NULL || node->refs < 2) {
    return;
	}
	PF->clear(out)->addi(out, 0); //retry
}

static void do_vfs_pipe_read(int pid, proto_t* in, proto_t* out) {
	(void)pid;
	fsinfo_t info;
	PF->addi(out, -1);
	if(proto_read_to(in, &info, sizeof(fsinfo_t)) != sizeof(fsinfo_t)) {
		return;
	}

  vfs_node_t* node = (vfs_node_t*)info.node;
	proc_wakeup((int32_t)node); //wakeup writer.

	int32_t size = proto_read_int(in);
	if(size < 0) {
		return;
	}

	buffer_t* buffer = (buffer_t*)info.data;
	if(buffer == NULL) {
		PF->clear(out)->addi(out, 0); // pipe still waiting for other-port, retry
		return;
	}

	void* data = malloc(size);
	size = buffer_read(buffer, data, size);
	if(size > 0) {
		PF->clear(out)->addi(out, size)->add(out, data, size);
		free(data);
		return;
	}
	free(data);

	if(node == NULL || node->refs < 2) {
    return;
	}
	PF->clear(out)->addi(out, 0); //retry
}

static void do_vfs_proc_clone(int32_t pid, proto_t* in) {
	(void)pid;
	int fpid = proto_read_int(in);
	int cpid = proto_read_int(in);
	if(fpid < 0 || cpid < 0)
		return;
	
	int32_t i;
	for(i=0; i<PROC_FILE_MAX; i++) {
		file_t *f = &_proc_fds_table[fpid].fds[i];
		vfs_node_t* node = 	(vfs_node_t*)f->node;
		if(node != NULL) {
			file_t* file = &_proc_fds_table[cpid].fds[i];
			memcpy(file, f, sizeof(file_t));
			node->refs++;
			if((f->flags & O_WRONLY) != 0)
				node->refs_w++;

			if(node->fsinfo.type == FS_TYPE_PIPE) {
				if(node->fsinfo.data == 0) {
					buffer_t* buf = (buffer_t*)malloc(sizeof(buffer_t));
					memset(buf, 0, sizeof(buffer_t));
					node->fsinfo.data = (int32_t)buf;
					proc_wakeup((int32_t)node);
				}
			}
		}
	}
}

static void do_vfs_proc_exit(int32_t pid, proto_t* in) {
	(void)pid;
	int cpid = proto_read_int(in);
	if(cpid < 0)
		return;
	
	int32_t i;
	for(i=0; i<PROC_FILE_MAX; i++) {
		file_t *f = &_proc_fds_table[cpid].fds[i];
		if(f->node != NULL) {
			proc_file_close(cpid, i, f, true);
		}
		memset(f, 0, sizeof(file_t));
	}
}

static void handle(int pid, int cmd, proto_t* in, proto_t* out, void* p) {
	(void)p;
	pid = proc_getpid(pid);

	switch(cmd) {
	case VFS_NEW_NODE:
		do_vfs_new_node(in, out);
		break;
	case VFS_OPEN:
		do_vfs_open(pid, in, out);
		break;
	case VFS_PIPE_OPEN:
		do_vfs_pipe_open(pid, out);
		break;
	case VFS_PIPE_WRITE:
		do_vfs_pipe_write(pid, in, out);
		break;
	case VFS_PIPE_READ:
		do_vfs_pipe_read(pid, in, out);
		break;
	case VFS_DUP:
		do_vfs_dup(pid, in, out);
		break;
	case VFS_DUP2:
		do_vfs_dup2(pid, in, out);
		break;
	case VFS_CLOSE:
		do_vfs_close(pid, in);
		break;
	case VFS_TELL:
		do_vfs_tell(pid, in, out);
		break;
	case VFS_SEEK:
		do_vfs_seek(pid, in, out);
		break;
	case VFS_GET_BY_NAME:
		do_vfs_get_by_name(in, out);
		break;
	case VFS_GET_BY_FD:
		do_vfs_get_by_fd(pid, in, out);
		break;
	case VFS_GET_FLAGS:
		do_vfs_get_flags(pid, in, out);
		break;
	case VFS_SET_FLAGS:
		do_vfs_set_flags(pid, in, out);
		break;
	case VFS_SET_FSINFO:
		do_vfs_set_fsinfo(pid, in, out);
		break;
	case VFS_ADD:
		do_vfs_add(pid, in, out);
		break;
	case VFS_DEL:
		do_vfs_del(pid, in, out);
		break;
	case VFS_MOUNT:
		do_vfs_mount(pid, in, out);
		break;
	case VFS_UMOUNT:
		do_vfs_umount(pid, in);
		break;
	case VFS_GET_KIDS:
		do_vfs_get_kids(in, out);
		break;
	case VFS_GET_MOUNT_BY_ID:
		do_vfs_get_mount_by_id(in, out);
		break;
	case VFS_PROC_CLONE:
		do_vfs_proc_clone(pid, in);
		break;
	case VFS_PROC_EXIT:
		do_vfs_proc_exit(pid, in);
		break;
	}
}

static void handle_close_event(close_event_t* ev) {
	proto_t in;
	PF->init(&in)->
			addi(&in, ev->fd)->
			addi(&in, ev->owner_pid)->
			add(&in, &ev->info, sizeof(fsinfo_t));
	ipc_call(ev->dev_pid, FS_CMD_CLOSE, &in, NULL);
	PF->clear(&in);
}

int vfsd_main(void) {
	_event_head = _event_tail = NULL;

	if(ipc_serv_reg(IPC_SERV_VFS) != 0) {
		klog("reg vfs ipc_serv error!\n");
		return -1;
	}

	vfs_init();

	ipc_serv_run(handle, NULL, IPC_NON_BLOCK);
	close_event_t ev;
	while(true) {
		//ipc_lock();
		int res = get_close_event(&ev);
		//ipc_unlock();
		if( res == 0)
			handle_close_event(&ev);
		else
			usleep(50000);
	}
	return 0;
}

