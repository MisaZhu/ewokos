#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/errno.h>
#include <ewoksys/ipc.h>
#include <ewoksys/klog.h>
#include <ewoksys/proc.h>
#include <ewoksys/mstr.h>
#include <ewoksys/buffer.h>
#include <ewoksys/proto.h>
#include <ewoksys/fsinfo.h>
#include <ewoksys/vfsc.h>
#include <ewoksys/syscall.h>
#include <ewoksys/hashmap.h>
#include <procinfo.h>
#include <sysinfo.h>

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
} file_t;

static vfs_node_t* _vfs_root = NULL;
static mount_t _vfs_mounts[FS_MOUNT_MAX];
static map_t  _nodes_hash = NULL;

typedef struct {
	file_t fds[MAX_OPEN_FILE_PER_PROC];
	uint32_t state;
	uint32_t uuid;
} proc_fds_t;

static proc_fds_t* _proc_fds_table = NULL;
static uint32_t    _max_proc_table_num = 0;

static void vfs_node_init(vfs_node_t* node) {
	memset(node, 0, sizeof(vfs_node_t));
	node->fsinfo.node = (uint32_t)node;
	node->mount_id = -1;
}

static inline const char* node_hash_key(uint32_t node_id) {
	static char key[32];
	snprintf(key, 31, "%x", (uint32_t)node_id);
	return key;
}

static vfs_node_t* vfs_new_node(void) {
	vfs_node_t* ret = (vfs_node_t*)malloc(sizeof(vfs_node_t));
	vfs_node_init(ret);

	hashmap_put(_nodes_hash, node_hash_key((uint32_t)ret), ret);
	return ret;
}

static vfs_node_t* vfs_get_node_by_id(uint32_t node_id) {
	vfs_node_t* node = NULL;
	hashmap_get(_nodes_hash, node_hash_key(node_id), (void**)&node);
	return node;
}

static void vfsd_init(void) {
	int32_t i;
	for(i = 0; i<FS_MOUNT_MAX; i++) {
		memset(&_vfs_mounts[i], 0, sizeof(mount_t));
	}

	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (ewokos_addr_t)&sysinfo);
	_max_proc_table_num = sysinfo.max_task_num;
	_proc_fds_table = (proc_fds_t*)malloc(_max_proc_table_num*sizeof(proc_fds_t));

	for(i = 0; i<_max_proc_table_num; i++) {
		memset(&_proc_fds_table[i], 0, sizeof(proc_fds_t));
	}

	_nodes_hash = hashmap_new();
	_vfs_root = vfs_new_node();
	strcpy(_vfs_root->fsinfo.name, "/");
}

static file_t* vfs_get_file(int32_t pid, int32_t fd) {
	if(pid < 0 || pid >= _max_proc_table_num || fd < 0 || fd >= MAX_OPEN_FILE_PER_PROC)
		return NULL;
	return &_proc_fds_table[pid].fds[fd];
}

static file_t* vfs_check_fd(int32_t pid, int32_t fd) {
	file_t* f = vfs_get_file(pid, fd);
	if(f->node == NULL)
		return NULL;
	return f;
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
	int32_t mnt_pid = get_mount_pid(node);
	if(mnt_pid != pid) //current proc not the mounting one.
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

static int32_t vfs_add_node(int32_t pid, vfs_node_t* father, vfs_node_t* node) {
	(void)pid;
	if(father == NULL || node == NULL)
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
	(void)pid;
	if(node == NULL) 
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

	static char ret[FS_FULL_NAME_MAX] = {0};
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
		vfs_add_node(pid, father, node);
	}
	return 0;
}

static void vfs_umount(int32_t pid, vfs_node_t* node) {
	if(node == NULL || node->mount_id < 0 || check_mount(pid, node) != 0)
		return;

	vfs_node_t* org = vfs_get_node_by_id(_vfs_mounts[node->mount_id].org_node);
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
			vfs_add_node(pid, father, org);
	}
	memset(&_vfs_mounts[node->mount_id], 0, sizeof(mount_t));
}

static int32_t vfs_del_node(vfs_node_t* node) {
	if(node == NULL || node->refs > 0)
		return -1;
	/*free children*/
	vfs_node_t* c = node->first_kid;
	vfs_node_t* father = node->father;

	while(c != NULL) {
		vfs_node_t* next = c->next;
		vfs_del_node(c);
		c = next;
	}

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
	hashmap_remove(_nodes_hash, node_hash_key((uint32_t)node));
	free(node);
	return 0;
}

static int32_t set_node_info(int32_t pid, vfs_node_t* node, fsinfo_t* info) {
	(void)pid;
	if(node == NULL || info == NULL)
		return -1;
	memcpy(&node->fsinfo, info, sizeof(fsinfo_t));
	return 0;
}

vfs_node_t* vfs_root(void) {
	return _vfs_root;
}

static int32_t get_free_fd(int32_t pid) {
	int32_t i;
	for(i=3; i<MAX_OPEN_FILE_PER_PROC; i++) { //0, 1, 2 reserved for stdio in/out/err
		if(_proc_fds_table[pid].fds[i].node == 0)
			return i;
	}
	return -1;
}

static int32_t vfs_open(int32_t pid, vfs_node_t* node, int32_t flags) {
	if(node == NULL)
		return -1;

	int32_t fd = get_free_fd(pid);
	if(fd < 0)
		return -1;

	_proc_fds_table[pid].fds[fd].node = node;
	_proc_fds_table[pid].fds[fd].flags = flags;

	if((flags & O_TRUNC) != 0)
		node->fsinfo.stat.size = 0;

	node->refs++;
	if((flags & O_WRONLY) != 0)
		node->refs_w++;
	return fd;
}

static vfs_node_t* vfs_open_announimous(int32_t pid, vfs_node_t* node) {
	if(node == NULL)
		return NULL;

	procinfo_t procinfo;
	if(proc_info(pid, &procinfo) != 0)
		return NULL;

	vfs_node_t* ret = vfs_new_node();
	ret->fsinfo.type = node->fsinfo.type;
	ret->fsinfo.stat.mode = 0700;
	ret->fsinfo.stat.uid = procinfo.uid;
	ret->fsinfo.stat.gid = procinfo.gid;
	ret->fsinfo.data = 0;
	ret->mount_id = node->mount_id;
	return ret;
}

static vfs_node_t* vfs_get_by_fd(int32_t pid, int32_t fd) {
	file_t* file = vfs_check_fd(pid, fd);
	if(file == NULL)
		return NULL;
	return file->node;
}

static void proc_file_close(int pid, int fd, file_t* file) {
	(void)pid;
	(void)fd;
	if(file == NULL || file->node == NULL)
		return;
	uint32_t node_id = (uint32_t)file->node;
	vfs_node_t* node = vfs_get_node_by_id(node_id);
	if(node == NULL)
		return;

	if(node->refs > 0)
		node->refs--;
	if((file->flags & (O_WRONLY|O_RDWR)) != 0 && node->refs_w > 0)
		node->refs_w--;
	bool del_node = false;
	if(node->fsinfo.type == FS_TYPE_PIPE) {
		if(node->refs <= 0) {
			if(node->fsinfo.name[0] == 0) { //no refs and not fifo pipe
				buffer_t* buffer = (buffer_t*)node->fsinfo.data;
				if(buffer != NULL)
					free(buffer);
				del_node = true;
				file->node = 0;
			}
			else {
				node->fsinfo.state = 0;
			}
		}
		proc_wakeup(node_id);
	}
	else if((node->fsinfo.type & FS_TYPE_ANNOUNIMOUS) != 0) {
		if(node->refs <= 0) {
			del_node = true;
			file->node = 0;
		}
		proc_wakeup(node_id);
	}

	if(del_node)
		vfs_del_node(node);
}

static void vfs_close(int32_t pid, int32_t fd) {
	if(pid < 0 || fd < 0 || fd >= MAX_OPEN_FILE_PER_PROC)
		return;

	file_t* f = vfs_check_fd(pid, fd);
	if(f != NULL) {
		proc_file_close(pid, fd, f);
		memset(f, 0, sizeof(file_t));
	}
}

static vfs_node_t* vfs_dup(int32_t pid, int32_t from, int32_t *ret) {
	if(from < 0 || from > MAX_OPEN_FILE_PER_PROC)
		return NULL;
	int32_t to = get_free_fd(pid);
	if(to < 0)
		return NULL;

	file_t* f = vfs_check_fd(pid, from);
	if(f == NULL) 
		return NULL;

	file_t* f_to = vfs_get_file(pid, to);
	if(f_to == NULL)
		return NULL;

	memcpy(f_to, f, sizeof(file_t));
	f->node->refs++;
	if((f->flags & O_WRONLY) != 0)
		f->node->refs_w++;
	*ret = to;
	return f_to->node;
}

static vfs_node_t* vfs_dup2(int32_t pid, int32_t from, int32_t to) {
	file_t* f = vfs_check_fd(pid, from);
	if(f == NULL) 
		return NULL;

	if(from == to)
		return f->node;

	if(from < 0 || from > MAX_OPEN_FILE_PER_PROC ||
			to < 0 || to > MAX_OPEN_FILE_PER_PROC)
		return NULL;

	vfs_close(pid, to);
	file_t* f_to = vfs_get_file(pid, to);
	if(f_to == NULL)
		return NULL;

	memcpy(f_to, f, sizeof(file_t));
	f->node->refs++;
	if((f->flags & O_WRONLY) != 0)
		f->node->refs_w++;
	return f->node;
}

static inline fsinfo_t* gen_fsinfo(vfs_node_t* node) {
	if(node == NULL)
		return NULL;

	static fsinfo_t ret;
	memcpy(&ret, &node->fsinfo, sizeof(fsinfo_t));
	ret.mount_pid = get_mount_pid(node);
	return &ret;
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

static void do_vfs_get_by_name(int32_t pid, proto_t* in, proto_t* out) {
	(void)pid;
	PF->addi(out, 0);
	const char* name = proto_read_str(in);
  vfs_node_t* node = vfs_get_by_name(vfs_root(), name);
  //if(node == NULL || vfs_check_access(pid, &node->fsinfo, R_OK) != 0)
  if(node == NULL)
    return;

	PF->clear(out)->addi(out, (int32_t)node)->add(out, gen_fsinfo(node), sizeof(fsinfo_t));
}

static void do_vfs_get_by_fd(int pid, proto_t* in, proto_t* out) {
	int fd = proto_read_int(in);
  vfs_node_t* node = vfs_get_by_fd(pid, fd);
  if(node == NULL) {
		PF->addi(out, 0);
    return;
	}

	PF->addi(out, (int32_t)node)->add(out, gen_fsinfo(node), sizeof(fsinfo_t));
}

static void do_vfs_get_by_node(int32_t pid, proto_t* in, proto_t* out) {
	(void)pid;
	PF->addi(out, 0);
	uint32_t node_id = (uint32_t)proto_read_int(in);
	if(node_id == 0)
		return;

  vfs_node_t* node = vfs_get_node_by_id(node_id);
	if(node == NULL || vfs_check_access(pid, &node->fsinfo, R_OK) != 0)
    return;

	PF->clear(out)->addi(out, (int32_t)node)->add(out, gen_fsinfo(node), sizeof(fsinfo_t));
}

static void do_vfs_new_node(int pid, proto_t* in, proto_t* out) {
	PF->addi(out, -1);
	fsinfo_t info;
	if(proto_read_to(in, &info, sizeof(fsinfo_t)) != sizeof(fsinfo_t))
		return;
	uint32_t node_to_id = (uint32_t)proto_read_int(in);
	bool vfs_node_only = (bool)proto_read_int(in);

	vfs_node_t* node_to = NULL;
	if(node_to_id > 0) {
		node_to = vfs_get_node_by_id(node_to_id);
		if (node_to == NULL) {
			PF->addi(out, ENOENT);
			return;
		}

		if(!vfs_node_only) {
			if(vfs_check_access(pid, &node_to->fsinfo, W_OK) != 0 ||
					vfs_check_access(pid, &node_to->fsinfo, X_OK) != 0) {
				PF->addi(out, EPERM);
				return;
			}
		}
	
		if(vfs_get_by_name(node_to, info.name) != NULL) {//existed ! 
			PF->addi(out, EEXIST);
			return;
		}
	}

 	vfs_node_t* node = vfs_new_node();
 	if(node == NULL)
		return;
	info.node = (uint32_t)node;
	info.mount_pid = -1;
	memcpy(&node->fsinfo, &info, sizeof(fsinfo_t));
	if(info.type == FS_TYPE_PIPE)  {
		buffer_t* buf = (buffer_t*)malloc(sizeof(buffer_t));
		memset(buf, 0, sizeof(buffer_t));
		node->fsinfo.data = (int32_t)buf;
	}

	if(node_to != NULL)
		vfs_add_node(pid, node_to, node);

	PF->clear(out)->addi(out, 0)->add(out, &info, sizeof(fsinfo_t));
}

static void do_vfs_open(int32_t pid, proto_t* in, proto_t* out) {
	PF->addi(out, -1);
	fsinfo_t info;
	if(proto_read_to(in, &info, sizeof(fsinfo_t)) != sizeof(fsinfo_t))
		return;

	int32_t flags = proto_read_int(in);
 	vfs_node_t* node = vfs_get_node_by_id(info.node);
 	if(node == NULL) {
		PF->addi(out, ENOENT);
		return;
	}

	if(((flags & O_WRONLY) != 0 ||
			(flags & O_RDWR) != 0) &&
			vfs_check_access(pid, &node->fsinfo, W_OK) != 0) {
		PF->addi(out, EPERM);
		return;
	}

	if(vfs_check_access(pid, &node->fsinfo, R_OK) != 0) {
		PF->addi(out, EPERM);
		return;
	}

	int res = -1;
	if((node->fsinfo.type & FS_TYPE_ANNOUNIMOUS) == 0)
		res = vfs_open(pid, node, flags);
	else {
		node = vfs_open_announimous(pid, node);
		if(node != NULL)
			res = vfs_open(pid, node, flags);
	}

	memcpy(&info, gen_fsinfo(node), sizeof(fsinfo_t));
	PF->clear(out);
	PF->addi(out, res)->add(out, &info, sizeof(fsinfo_t));
}

static void do_vfs_close(int32_t pid, proto_t* in) {
	int fd = proto_read_int(in);
	if(fd < 0)
		return;
	vfs_close(pid, fd);
}

static void do_vfs_dup(int32_t pid, proto_t* in, proto_t* out) {
	int fd = proto_read_int(in);
	int32_t fdto = -1;
	vfs_node_t* node = vfs_dup(pid, fd, &fdto);
	if(node != NULL)
		PF->addi(out, fdto)->add(out, gen_fsinfo(node), sizeof(fsinfo_t));
	else
		PF->addi(out, -1);
}

static void do_vfs_dup2(int32_t pid, proto_t* in, proto_t* out) {
	int32_t fd = proto_read_int(in);
	int32_t fdto = proto_read_int(in);

	vfs_node_t* node = vfs_dup2(pid, fd, fdto);
	if(node != NULL)
		PF->addi(out, fdto)->add(out, gen_fsinfo(node), sizeof(fsinfo_t));
	else
		PF->addi(out, -1);
}

static void do_vfs_set_fsinfo(int32_t pid, proto_t* in, proto_t* out) {
	fsinfo_t info;
	PF->addi(out, -1);
	int res = -1;
	if(proto_read_to(in, &info, sizeof(fsinfo_t)) == sizeof(fsinfo_t)) {
		vfs_node_t* node = vfs_get_node_by_id(info.node);
		if(node == NULL) { 
			PF->addi(out, ENOENT);
			return;
		}
  		if(vfs_check_access(pid, &node->fsinfo, W_OK) != 0) {
			PF->addi(out, EPERM);
			return;
		}
		res = set_node_info(pid, node, &info);
	}
	PF->clear(out)->addi(out, res);
}

static void do_vfs_get_kids(int pid, proto_t* in, proto_t* out) {
	PF->addi(out, 0);
	uint32_t node_id = (uint32_t)proto_read_int(in);
	if(node_id == 0)
		return;

	vfs_node_t* node = vfs_get_node_by_id(node_id);
	if(node == NULL)
 	   return;

	if(vfs_check_access(pid, &node->fsinfo, X_OK) != 0)
		return;
	
	uint32_t num = 0;
	fsinfo_t* kids = vfs_get_kids(node, &num);
	if(kids == NULL || num == 0)
		return;
	PF->clear(out)->addi(out, num)->add(out, kids, sizeof(fsinfo_t)*num);
}

static void do_vfs_del_node(int32_t pid, proto_t* in, proto_t* out) {
	PF->addi(out, -1);
	uint32_t node_id = proto_read_int(in);
	if(node_id == 0)
		return;

	vfs_node_t *node = vfs_get_node_by_id(node_id);
	if (node == NULL)
		return;

	int res = vfs_del_node(node);
	PF->clear(out)->addi(out, res);
}

static void do_vfs_mount(int32_t pid, proto_t* in, proto_t* out) {
	uint32_t node_to_id;
	uint32_t node_id;

	PF->addi(out, -1);
	node_to_id = (uint32_t)proto_read_int(in);
	if(node_to_id == 0)
		return;
	node_id = (uint32_t)proto_read_int(in);
	if(node_id == 0)
		return;

	vfs_node_t* node_to = vfs_get_node_by_id(node_to_id);
	vfs_node_t* node = vfs_get_node_by_id(node_id);
	if(node_to == NULL || node == NULL)
    return;

	vfs_mount(pid, node_to, node);
	PF->clear(out)->addi(out, 0);
}

static void do_vfs_umount(int32_t pid, proto_t* in) {
	uint32_t node_id = (uint32_t)proto_read_int(in);
	if(node_id == 0)
		return;

  vfs_node_t* node = vfs_get_node_by_id(node_id);
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
	PF->addi(out, -1);

	procinfo_t procinfo;
	if(proc_info(pid, &procinfo) != 0)
		return;

	vfs_node_t* node = vfs_new_node();
	if(node == NULL)
		return;
	
	node->fsinfo.type = FS_TYPE_PIPE;
	node->fsinfo.stat.mode = 0666;
	node->fsinfo.stat.uid = procinfo.uid;
	node->fsinfo.stat.gid = procinfo.gid;

	buffer_t* buf = (buffer_t*)malloc(sizeof(buffer_t));
	memset(buf, 0, sizeof(buffer_t));
	node->fsinfo.data = (int32_t)buf;

	int32_t fd0 = vfs_open(pid, node, 1);
	if(fd0 < 0) {
		vfs_del_node(node);
		return;
	}

	int32_t fd1 = vfs_open(pid, node, 1);
	if(fd1 < 0) {
		vfs_close(pid, fd0);
		vfs_del_node(node);
		return;
	}
	PF->clear(out)->addi(out, 0)->
			addi(out, fd0)->
			addi(out, fd1)->
			add(out, gen_fsinfo(node), sizeof(fsinfo_t));
}

static void do_vfs_pipe_write(int pid, proto_t* in, proto_t* out) {
	(void)pid;
	PF->addi(out, -1);

	int32_t fd = proto_read_int(in);
	if(vfs_check_fd(pid, fd) == NULL)
		return;

	uint32_t node_id = proto_read_int(in);
	if(node_id == 0)
		return;

	int32_t size = 0;
	void *data = proto_read(in, &size);
	vfs_node_t* node = vfs_get_node_by_id(node_id);

	//if(size < 0 || data == NULL || node == NULL || node->refs < 2) { //closed by other peer
	if(size < 0 || data == NULL || node == NULL) { //closed by other peer
		proc_wakeup(node_id); //wakeup reader
		return;
	}

	buffer_t* buffer = (buffer_t*)node->fsinfo.data;
	if(buffer == NULL) { //pipe buffer not ready 
		proc_wakeup(node_id); //wakeup reader
		return;
	}

	size = buffer_write(buffer, data, size);
	if(size > 0) {
		node->fsinfo.state |= FS_STATE_CHANGED;
		PF->clear(out)->addi(out, size);
		proc_wakeup(node_id); //wakeup reader
		return;
	}

	PF->clear(out)->addi(out, 0); //buffer full(waiting for read), retry
}

static void do_vfs_pipe_read(int pid, proto_t* in, proto_t* out) {
	(void)pid;
	PF->addi(out, -1);

	int32_t fd = proto_read_int(in);
	if(vfs_check_fd(pid, fd) == NULL)
		return;

	uint32_t node_id = proto_read_int(in);
	if(node_id == 0)
		return;

	vfs_node_t* node = vfs_get_node_by_id(node_id);
	int32_t size = proto_read_int(in);

	buffer_t* buffer = (buffer_t*)node->fsinfo.data;
	if(buffer == NULL) { //buffer not ready 
		proc_wakeup(node_id); //wakeup writer.
		return;
	}

	if(node == NULL || size < 0 || 
					(buffer->size == 0 &&
					(node->refs_w == 0 || node->refs < 2) && 
					(node->fsinfo.state & FS_STATE_CHANGED) != 0 )) { // close by other peer
		proc_wakeup(node_id); //wakeup writer.
   	return;
	}

	if(buffer->size > 0 && size > 0) {
		void* data = malloc(size);
		size = buffer_read(buffer, data, size);
		if(size > 0) {
			PF->clear(out)->addi(out, size)->add(out, data, size);
			free(data);
			proc_wakeup(node_id); //wakeup writer.
			return;
		}
		free(data);
	}

	PF->clear(out)->addi(out, 0); //retry
}

static void vfs_proc_exit(int32_t cpid) {
	if(cpid < 0)
		return;
	int32_t i;
	for(i=0; i<MAX_OPEN_FILE_PER_PROC; i++) {
		file_t *f = &_proc_fds_table[cpid].fds[i];
		if(f->node != NULL) {
			proc_file_close(cpid, i, f);
		}
		memset(f, 0, sizeof(file_t));
	}
	_proc_fds_table[cpid].state = UNUSED;
	_proc_fds_table[cpid].uuid = 0;
}

static void do_vfs_proc_clone(int32_t pid, proto_t* in) {
	(void)pid;
	int fpid = proto_read_int(in);
	int cpid = proto_read_int(in);
	if(fpid < 0 || cpid < 0)
		return;

	if(_proc_fds_table[cpid].state == RUNNING)
		vfs_proc_exit(cpid);
	_proc_fds_table[cpid].state = RUNNING;
	_proc_fds_table[cpid].uuid = proc_get_uuid(cpid);
	
	int32_t i;
	for(i=0; i<MAX_OPEN_FILE_PER_PROC; i++) {
		file_t *f = &_proc_fds_table[fpid].fds[i];
		vfs_node_t* node = 	vfs_get_node_by_id((uint32_t)f->node);
		if(node != NULL) {
			file_t* file = &_proc_fds_table[cpid].fds[i];
			memcpy(file, f, sizeof(file_t));
			node->refs++;
			if((f->flags & O_WRONLY) != 0)
				node->refs_w++;
		}
	}
}

/*
static void check_procs(void) {
	int32_t i;
	for(i = 0; i<_max_proc_table_num; i++) {
		if(_proc_fds_table[i].state != UNUSED) {
			if(proc_get_uuid(i) != _proc_fds_table[i].uuid) {
				vfs_proc_exit(i);
			}
		}
	}
}
*/

static void do_vfs_proc_exit(int32_t pid, proto_t* in) {
	(void)pid;
	int cpid = proto_read_int(in);
	if(cpid < 0 || cpid >= _max_proc_table_num)
		return;
	vfs_proc_exit(cpid);
}

static void handle(int pid, int cmd, proto_t* in, proto_t* out, void* p) {
	(void)p;
	if(_proc_fds_table[pid].state == UNUSED) //maybe thread 
		pid = proc_getpid(pid); //get the main proc pid
	if(pid < 0)
		return;

	//klog("pid: %d, cmd: %d\n", pid, cmd);

	switch(cmd) {
	case VFS_NEW_NODE:
		do_vfs_new_node(pid, in, out);
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
	case VFS_GET_BY_NAME:
		do_vfs_get_by_name(pid, in, out);
		break;
	case VFS_GET_BY_NODE:
		do_vfs_get_by_node(pid, in, out);
		break;
	case VFS_GET_BY_FD:
		do_vfs_get_by_fd(pid, in, out);
		break;
	case VFS_SET_FSINFO:
		do_vfs_set_fsinfo(pid, in, out);
		break;
	case VFS_DEL_NODE:
		do_vfs_del_node(pid, in, out);
		break;
	case VFS_MOUNT:
		do_vfs_mount(pid, in, out);
		break;
	case VFS_UMOUNT:
		do_vfs_umount(pid, in);
		break;
	case VFS_GET_KIDS:
		do_vfs_get_kids(pid, in, out);
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
	//klog("vfs cmd done\n");
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	if(ipc_serv_reg(IPC_SERV_VFS) != 0) {
		klog("reg vfs ipc_serv error!\n");
		return -1;
	}

	vfsd_init();

	ipc_serv_run(handle, NULL, NULL, IPC_DEFAULT);

	while(true) {
		proc_block_by(getpid(), (uint32_t)main);
	}

	free(_proc_fds_table);
	return 0;
}