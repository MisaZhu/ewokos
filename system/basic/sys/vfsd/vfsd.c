#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/errno.h>
#include <ewoksys/ipc.h>
#include <ewoksys/klog.h>
#include <ewoksys/vfsc.h>
#include <ewoksys/proc.h>
#include <ewoksys/mstr.h>
#include <ewoksys/buffer.h>
#include <ewoksys/proto.h>
#include <ewoksys/fsinfo.h>
#include <ewoksys/vfsc.h>
#include <ewoksys/sys.h>
#include <ewoksys/syscall.h>
#include <ewoksys/hashmap.h>
#include <ewoksys/queue.h>
#include <procinfo.h>
#include <sysinfo.h>

typedef struct vfs_node {
  struct vfs_node* father;
  struct vfs_node* first_kid; /*first child*/
  struct vfs_node* last_kid; /*last child*/
  struct vfs_node* next; /*next brother*/
  struct vfs_node* prev; /*prev brother*/
  void* data_ptr;
  uint32_t node_id;
  uint32_t kids_num;

  fsinfo_t fsinfo;

  int32_t mount_id;
  uint32_t refs;
  uint32_t refs_w;
  uint32_t events; //events for poll or select
  queue_t read_wait_queue;
  queue_t write_wait_queue;
} vfs_node_t;

typedef struct {
  vfs_node_t *node;
  uint32_t flags;
  fsinfo_t fsinfo;
} file_t;

static vfs_node_t* _vfs_root = NULL;
static mount_t _vfs_mounts[FS_MOUNT_MAX];
static map_t  _nodes_hash = NULL;
static uint32_t _next_node_id = 1;

typedef struct {
	uint32_t uuid;
	uint32_t state;
	file_t fds[MAX_OPEN_FILE_PER_PROC];
} proc_fds_t;

typedef struct {
	int32_t pid;
	uint32_t uuid;
} wait_entry_t;

static proc_fds_t* _proc_fds_table = NULL;
static uint32_t    _max_proc_table_num = 0;

static uint32_t vfs_alloc_node_id(void) {
	uint32_t node_id = _next_node_id++;
	if(node_id == 0) {
		node_id = _next_node_id++;
	}
	return node_id;
}

static void vfs_node_init(vfs_node_t* node) {
	memset(node, 0, sizeof(vfs_node_t));
	node->node_id = vfs_alloc_node_id();
	node->fsinfo.node = node->node_id;
	node->mount_id = -1;
	queue_init(&node->read_wait_queue);
	queue_init(&node->write_wait_queue);
}

static uint32_t vfs_get_node_id(vfs_node_t* node) {
	if(node == NULL)
		return 0;
	return node->node_id;
}

static inline const char* node_hash_key(uint32_t node_id) {
	static char key[32];
	snprintf(key, 31, "%x", (uint32_t)node_id);
	return key;
}

static vfs_node_t* vfs_new_node(void) {
	vfs_node_t* ret = (vfs_node_t*)malloc(sizeof(vfs_node_t));
	vfs_node_init(ret);

	hashmap_put(_nodes_hash, node_hash_key(ret->node_id), ret);
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
	sys_get_sys_info(&sysinfo);
	_max_proc_table_num = sysinfo.max_task_num;
	_proc_fds_table = (proc_fds_t*)malloc(_max_proc_table_num*sizeof(proc_fds_t));

	for(i = 0; i<_max_proc_table_num; i++) {
		memset(&_proc_fds_table[i], 0, sizeof(proc_fds_t));
	}

	_nodes_hash = hashmap_new(0);
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
	if(f->node == NULL) {
		pid = proc_getpid(pid);
		if(pid < 0)
			return NULL;

		f = vfs_get_file(pid, fd);
		if(f->node == NULL)
			return NULL;
	}
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

static int32_t vfs_get_mount_pid(vfs_node_t* node) {
	int32_t mount_id = vfs_get_mount_id(node);
	if(mount_id < 0)
		return -1;
	return _vfs_mounts[mount_id].pid;
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
			if(name[i+1] == 0)
				return node;
			if(node == NULL)
				return NULL;
			j = i+1;
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
	_vfs_mounts[id].org_node = vfs_get_node_id(org);
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
	queue_clear(&node->read_wait_queue, free);
	queue_clear(&node->write_wait_queue, free);
	hashmap_remove(_nodes_hash, node_hash_key(node->node_id));
	free(node);
	return 0;
}

static int32_t set_node_info(int32_t pid, vfs_node_t* node, fsinfo_t* info) {
	(void)pid;
	if(node == NULL || info == NULL)
		return -1;
	uint32_t node_id = node->fsinfo.node;
	memcpy(&node->fsinfo, info, sizeof(fsinfo_t));
	node->fsinfo.node = node_id;
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
	memcpy(&_proc_fds_table[pid].fds[fd].fsinfo, &node->fsinfo, sizeof(fsinfo_t));
	_proc_fds_table[pid].fds[fd].fsinfo.node = vfs_get_node_id(node);
	_proc_fds_table[pid].fds[fd].fsinfo.mount_pid = get_mount_pid(node);

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

static inline fsinfo_t* gen_file_fsinfo(file_t* file) {
	static fsinfo_t ret;
	if(file == NULL || file->node == NULL)
		return NULL;
	memcpy(&ret, &file->node->fsinfo, sizeof(fsinfo_t));
	ret.data = file->fsinfo.data;
	ret.node = vfs_get_node_id(file->node);
	ret.mount_pid = get_mount_pid(file->node);
	return &ret;
}

static void wakeup_proc(wait_entry_t* waiter, vfs_node_t* node, int32_t events) {
	(void)node;
	(void)events;
	if(waiter == NULL)
		return;
	if(waiter->pid < 0 || waiter->pid >= _max_proc_table_num)
		return;
	if(proc_check_uuid(waiter->pid, waiter->uuid) == 0)
		return;
	proc_wakeup(waiter->pid);
}

static void wakeup_wait_queue(queue_t* q, vfs_node_t* node, int32_t events) {
	if(q == NULL)
		return;

	while(true) {
		wait_entry_t* waiter = (wait_entry_t*)queue_pop(q);
		if(waiter == NULL)
			break;
		wakeup_proc(waiter, node, events);
		free(waiter);
	}
}

static bool queue_waiter_match(void* data, void* check_data) {
	if(data == NULL || check_data == NULL)
		return false;
	wait_entry_t* waiter = (wait_entry_t*)data;
	wait_entry_t* check = (wait_entry_t*)check_data;
	return waiter->pid == check->pid && waiter->uuid == check->uuid;
}

static void remove_waiter_from_queue(queue_t* q, const wait_entry_t* waiter) {
	if(q == NULL || waiter == NULL)
		return;

	queue_item_t* it = q->head;
	while(it != NULL) {
		queue_item_t* next = it->next;
		if(queue_waiter_match(it->data, (void*)waiter)) {
			free(it->data);
			queue_remove(q, it);
		}
		it = next;
	}
}

static int remove_waiter_from_node(map_t in, const char* key, any_t value, any_t arg) {
	(void)in;
	(void)key;
	vfs_node_t* node = (vfs_node_t*)value;
	wait_entry_t* waiter = (wait_entry_t*)arg;
	if(node == NULL || waiter == NULL)
		return MAP_OK;

	remove_waiter_from_queue(&node->read_wait_queue, waiter);
	remove_waiter_from_queue(&node->write_wait_queue, waiter);
	return MAP_OK;
}

static void vfs_remove_proc_waiters(int32_t pid, uint32_t uuid) {
	if(pid < 0 || pid >= _max_proc_table_num || uuid == 0)
		return;

	wait_entry_t waiter;
	waiter.pid = pid;
	waiter.uuid = uuid;
	hashmap_iterate(_nodes_hash, remove_waiter_from_node, &waiter);
}

static void do_node_wakeup(vfs_node_t* node, int events) {
	if(node == NULL)
		return;

	node->events |= events;

	queue_t* qr = NULL, *qw = NULL;
	if((events & VFS_EVT_RD) != 0 ||
			events == VFS_EVT_CLOSE || events == VFS_EVT_ERR || events == VFS_EVT_NVAL)
		qr = &node->read_wait_queue;
	if((events & VFS_EVT_WR) != 0 ||
			events == VFS_EVT_CLOSE || events == VFS_EVT_ERR || events == VFS_EVT_NVAL)
		qw = &node->write_wait_queue;

	/*
	 * Duplicated/fork-inherited descriptors can leave multiple processes
	 * blocked on the same node event. Waking only one waiter lets it consume
	 * the sticky edge and can strand sibling waiters forever.
	 */
	wakeup_wait_queue(qr, node, events);
	wakeup_wait_queue(qw, node, events);
}

static void sync_pipe_poll_events(vfs_node_t* node) {
	if(node == NULL || node->fsinfo.type != FS_TYPE_PIPE)
		return;

	uint32_t events = node->events & ~(VFS_EVT_RD | VFS_EVT_WR);
	buffer_t* buffer = (buffer_t*)node->data_ptr;
	if(buffer != NULL) {
		if(buffer->size > 0)
			events |= VFS_EVT_RD;
		else
			events |= VFS_EVT_WR;
	}
	node->events = events;
}

static void vfs_driver_close(int32_t pid, int32_t fd, file_t* file) {
	if(file == NULL || file->node == NULL)
		return;
	vfs_node_t* node = file->node;
	uint32_t type = file->fsinfo.type & FS_TYPE_MASK;

	/*
	 * Regular filesystem objects in rootfs do not keep per-fd runtime state in
	 * the backing driver. Zombie cleanup already detached the VFS-side slot, so
	 * round-tripping a no-op FS_CMD_CLOSE for every inherited script/config file
	 * only lengthens the window where boot-time metadata traffic contends with
	 * reaping detached helpers.
	 */
	if(type == FS_TYPE_FILE || type == FS_TYPE_DIR || type == FS_TYPE_LINK)
		return;

	proto_t in;
	PF->format(&in, "i,i,m,i",
		fd, vfs_get_node_id(node), &file->fsinfo, sizeof(fsinfo_t), pid);
	int32_t mount_pid = vfs_get_mount_pid(node);
	if(mount_pid > 0) {
		ipc_call(mount_pid, FS_CMD_CLOSE, &in, NULL);	
	}
	PF->clear(&in);
}

static void vfs_driver_dup(int32_t from_pid, int32_t from_fd,
		int32_t dup_pid, int32_t dup_fd, file_t* file) {
	if(file == NULL || file->node == NULL)
		return;
	vfs_node_t* node = file->node;
	uint32_t type = file->fsinfo.type & FS_TYPE_MASK;

	/*
	 * Regular filesystem objects do not carry per-fd runtime state in their
	 * mount driver. Reads and writes always pass the current offset down from
	 * libc, and the generic vdevice layer can lazily rebuild its cache from the
	 * VFS node info on first access. Avoid synchronously round-tripping every
	 * fork/dup of rootfs script files through the backing fs server so a stuck
	 * FS_CMD_DUP cannot strand the parent and child in fork.
	 */
	if(type == FS_TYPE_FILE || type == FS_TYPE_DIR || type == FS_TYPE_LINK)
		return;

	proto_t in;
	PF->format(&in, "i,i,i,m,i,i",
		from_fd, dup_fd, vfs_get_node_id(node), &file->fsinfo, sizeof(fsinfo_t),
		from_pid, dup_pid);
	int32_t mount_pid = vfs_get_mount_pid(node);
	if(mount_pid > 0) {
		ipc_call(mount_pid, FS_CMD_DUP, &in, NULL);
	}
	PF->clear(&in);
}

static void proc_file_close(int pid, int fd, file_t* file) {
	(void)pid;
	(void)fd;
	if(file == NULL || file->node == NULL)
		return;
	vfs_node_t* node = file->node;
	if(node == NULL)
		return;

	if(node->refs > 0)
		node->refs--;
	if((file->flags & (O_WRONLY|O_RDWR)) != 0 && node->refs_w > 0)
		node->refs_w--;
	bool del_node = false;
	if(node->fsinfo.type == FS_TYPE_PIPE) {
		uint32_t read_refs = (node->refs >= node->refs_w) ? (node->refs - node->refs_w) : 0;
		bool peer_gone = (node->refs_w == 0 || read_refs == 0);
		if(peer_gone)
			node->events |= VFS_EVT_CLOSE;
		else
			node->events &= ~VFS_EVT_CLOSE;
		sync_pipe_poll_events(node);

		if(node->refs <= 0) {
			if(node->fsinfo.name[0] == 0) { //no refs and not fifo pipe
				buffer_t* buffer = (buffer_t*)node->data_ptr;
				if(buffer != NULL)
					free(buffer);
				node->data_ptr = NULL;
				del_node = true;
				file->node = 0;
			}
			else {
				node->fsinfo.state = 0;
			}
		}
		if(peer_gone)
			do_node_wakeup(node, VFS_EVT_CLOSE);
	}
	else if((node->fsinfo.type & FS_TYPE_ANNOUNIMOUS) != 0) {
		if(node->refs <= 0) {
			del_node = true;
			file->node = 0;
			do_node_wakeup(node, VFS_EVT_CLOSE);
		}
	}

	if(del_node)
		vfs_del_node(node);
	file->node = NULL;
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
	ret.node = vfs_get_node_id(node);
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
	if(node == NULL)
		return;
	PF->clear(out)->addi(out, vfs_get_node_id(node))->add(out, gen_fsinfo(node), sizeof(fsinfo_t));
}

static void do_vfs_get_by_fd(int pid, proto_t* in, proto_t* out) {
	int fd = proto_read_int(in);
	file_t* file = vfs_check_fd(pid, fd);
	if(file == NULL || file->node == NULL) {
		PF->addi(out, 0);
		return;
	}

	PF->addi(out, vfs_get_node_id(file->node))->add(out, gen_file_fsinfo(file), sizeof(fsinfo_t));
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

	PF->clear(out)->addi(out, vfs_get_node_id(node))->add(out, gen_fsinfo(node), sizeof(fsinfo_t));
}

static void do_vfs_new_node(int pid, proto_t* in, proto_t* out) {
	PF->addi(out, -1);
	fsinfo_t info;
	if(proto_read_to(in, &info, sizeof(fsinfo_t)) != sizeof(fsinfo_t))
		return;
	uint32_t node_to_id = (uint32_t)proto_read_int(in);
	bool vfs_node_only = (bool)proto_read_int(in);
	bool vfs_write_over = (bool)proto_read_int(in);

	vfs_node_t* node = NULL;
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
	
		node = vfs_get_by_name(node_to, info.name);
		if(node != NULL) {//existed ! 
			if(!vfs_write_over) {
				PF->addi(out, EEXIST);
				return;
			}
		}
		else {
			vfs_write_over = false;
		}
	}

	if(node == NULL) {
	 	node = vfs_new_node();
	 	if(node == NULL)
			return;
	}

	info.node = vfs_get_node_id(node);
	info.mount_pid = -1;
	memcpy(&node->fsinfo, &info, sizeof(fsinfo_t));
	if(info.type == FS_TYPE_PIPE)  {
		buffer_t* buf = (buffer_t*)malloc(sizeof(buffer_t));
		memset(buf, 0, sizeof(buffer_t));
		node->data_ptr = buf;
		node->fsinfo.data = 0;
	}

	if(node_to != NULL && !vfs_write_over)
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
	file_t* file = vfs_check_fd(pid, res);
	if(file != NULL) {
		memcpy(&info, gen_file_fsinfo(file), sizeof(fsinfo_t));
	}
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
	if(node != NULL) {
		file_t* file = vfs_check_fd(pid, fdto);
		vfs_driver_dup(pid, fd, pid, fdto, file);
		PF->addi(out, fdto)->add(out, gen_file_fsinfo(file), sizeof(fsinfo_t));
	}
	else
		PF->addi(out, -1);
}

static void do_vfs_dup2(int32_t pid, proto_t* in, proto_t* out) {
	int32_t fd = proto_read_int(in);
	int32_t fdto = proto_read_int(in);

	vfs_node_t* node = vfs_dup2(pid, fd, fdto);
	if(node != NULL) {
		file_t* file = vfs_check_fd(pid, fdto);
		vfs_driver_dup(pid, fd, pid, fdto, file);
		PF->addi(out, fdto)->add(out, gen_file_fsinfo(file), sizeof(fsinfo_t));
	}
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

static void do_vfs_set_by_fd(int32_t pid, proto_t* in, proto_t* out) {
	int fd = proto_read_int(in);
	fsinfo_t info;
	PF->addi(out, -1);
	if(proto_read_to(in, &info, sizeof(fsinfo_t)) != sizeof(fsinfo_t))
		return;
	file_t* file = vfs_check_fd(pid, fd);
	if(file == NULL || file->node == NULL)
		return;
	memcpy(&file->fsinfo, &info, sizeof(fsinfo_t));
	file->fsinfo.node = vfs_get_node_id(file->node);
	file->fsinfo.mount_pid = get_mount_pid(file->node);
	PF->clear(out)->addi(out, 0);
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
	node->data_ptr = buf;
	node->fsinfo.data = 0;
	sync_pipe_poll_events(node);

	int32_t fd0 = vfs_open(pid, node, O_RDONLY);
	if(fd0 < 0) {
		vfs_del_node(node);
		return;
	}

	int32_t fd1 = vfs_open(pid, node, O_WRONLY);
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
		do_node_wakeup(node, VFS_EVT_RD); //wakeup reader
		return;
	}

	buffer_t* buffer = (buffer_t*)node->data_ptr;
	if(buffer == NULL) { //pipe buffer not ready 
		sync_pipe_poll_events(node);
		do_node_wakeup(node, VFS_EVT_WR);; //wakeup reader
		return;
	}

	size = buffer_write(buffer, data, size);
	if(size > 0) {
		node->fsinfo.state |= FS_STATE_CHANGED;
		sync_pipe_poll_events(node);
		PF->clear(out)->addi(out, size);
		do_node_wakeup(node, VFS_EVT_RD); //wakeup reader
		return;
	}

	sync_pipe_poll_events(node);
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

	buffer_t* buffer = (buffer_t*)node->data_ptr;
	if(buffer == NULL) { //buffer not ready 
		sync_pipe_poll_events(node);
		do_node_wakeup(node, VFS_EVT_WR); //wakeup writer.
		return;
	}

	if(node == NULL || size < 0 || 
					(buffer->size == 0 &&
					(node->refs_w == 0 || node->refs < 2) && 
					(node->fsinfo.state & FS_STATE_CHANGED) != 0 )) { // close by other peer
		sync_pipe_poll_events(node);
		do_node_wakeup(node, VFS_EVT_WR); //wakeup writer.
   		return;
	}

	if(buffer->size > 0 && size > 0) {
		void* data = malloc(size);
		size = buffer_read(buffer, data, size);
		if(size > 0) {
			sync_pipe_poll_events(node);
			PF->clear(out)->addi(out, size)->add(out, data, size);
			free(data);
			do_node_wakeup(node, VFS_EVT_WR); //wakeup writer.
			return;
		}
		free(data);
	}

	sync_pipe_poll_events(node);
	PF->clear(out)->addi(out, 0); //retry
}


static void vfs_proc_exit(int32_t cpid) {
	if(cpid < 0 || cpid >= _max_proc_table_num)
		return;
	_proc_fds_table[cpid].state = ZOMBIE;
}

static void clear_zombie(int32_t cpid) {
	if(cpid < 0)
		return;
	uint32_t uuid = _proc_fds_table[cpid].uuid;
	vfs_remove_proc_waiters(cpid, uuid);

	int32_t i;
	for(i=0; i<MAX_OPEN_FILE_PER_PROC; i++) {
		file_t *f = &_proc_fds_table[cpid].fds[i];
		if(f->node != NULL) {
			/*
			 * Zombie cleanup runs from the main loop with IPC temporarily
			 * disabled to avoid re-entrant mutation of the fd tables. Do not
			 * keep vfsd globally unavailable while synchronously notifying
			 * backing drivers: detach the fd from the zombie table first, then
			 * re-enable IPC around the potentially blocking driver close so
			 * metadata lookups like VFS_GET_BY_NODE can still make progress.
			 */
			file_t closing = *f;
			memset(f, 0, sizeof(file_t));
			ipc_enable();
			vfs_driver_close(cpid, i, &closing);
			ipc_disable();
			proc_file_close(cpid, i, &closing);
			continue;
		}
		memset(f, 0, sizeof(file_t));
	}

	_proc_fds_table[cpid].state = UNUSED;
	_proc_fds_table[cpid].uuid = 0;
}

static void clear_zombies(void) {
	int32_t i;
	for(i = 0; i<_max_proc_table_num; i++) {
		if(_proc_fds_table[i].state == ZOMBIE) {
			/*
			 * Boot scripts spawn and detach many short-lived helpers. Scanning the
			 * whole proc table with IPC disabled stalls unrelated metadata requests
			 * behind zombie cleanup. Only hold the non-reentrant section while
			 * actually reclaiming one zombie slot.
			 */
			ipc_disable();
			if(_proc_fds_table[i].state == ZOMBIE) {
				clear_zombie(i);
			}
			ipc_enable();
		}
	}
}

static void do_vfs_proc_clone(int32_t pid, proto_t* in) {
	(void)pid;
	int fpid = proto_read_int(in);
	int cpid = proto_read_int(in);
	if(fpid < 0 || fpid >= _max_proc_table_num ||
			cpid < 0 || cpid >= _max_proc_table_num)
		return;

	if(_proc_fds_table[cpid].state == RUNNING)
		vfs_proc_exit(cpid);
	else if(_proc_fds_table[cpid].state == ZOMBIE)
		clear_zombie(cpid);

	_proc_fds_table[cpid].state = RUNNING;
	_proc_fds_table[cpid].uuid = proc_get_uuid(cpid);
	
	int32_t i;
	for(i=0; i<MAX_OPEN_FILE_PER_PROC; i++) {
		file_t *f = &_proc_fds_table[fpid].fds[i];
		vfs_node_t* node = f->node;
		if(node != NULL) {
			file_t* file = &_proc_fds_table[cpid].fds[i];
			memcpy(file, f, sizeof(file_t));
			node->refs++;
			if((f->flags & O_WRONLY) != 0)
				node->refs_w++;
			vfs_driver_dup(fpid, i, cpid, i, file);
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

static void do_vfs_block(int32_t pid, proto_t* in) {
	int node_id = proto_read_int(in);
	int events = proto_read_int(in);
	if(node_id == 0)
		return;
    vfs_node_t* node = vfs_get_node_by_id(node_id);
	if(node == NULL)
		return;

	queue_t* q = NULL;
	if((events & VFS_EVT_RD) != 0)
		q = &node->read_wait_queue;
	else if((events & VFS_EVT_WR) != 0)
		q = &node->write_wait_queue;

	if(q == NULL)
		return;

	if((node->events & (uint32_t)events) != 0)
		return;

	wait_entry_t waiter;
	waiter.pid = pid;
	waiter.uuid = proc_get_uuid(pid);
	if(waiter.uuid == 0)
		return;

	if(queue_in(q, &waiter, queue_waiter_match) != NULL)
		return;

	wait_entry_t* p = (wait_entry_t*)malloc(sizeof(wait_entry_t));
	if(p == NULL)
		return;
	*p = waiter;
	queue_push(q, p);
}

static void do_vfs_wakeup(int32_t pid, proto_t* in) {
	(void)pid;
	int node_id = proto_read_int(in);
	int events = proto_read_int(in);
	if(node_id == 0)
		return;
    vfs_node_t* node = vfs_get_node_by_id(node_id);
	do_node_wakeup(node, events);
}

static void do_vfs_get_poll_events(int32_t pid, proto_t* in, proto_t* out) {
	PF->addi(out, 0);
	if(pid < 0 || pid >= _max_proc_table_num) {
		return;
	}
	uint32_t node_id = (uint32_t)proto_read_int(in);
	vfs_node_t* node = vfs_get_node_by_id(node_id);
	if(node == NULL)
		return;
	PF->clear(out)->addi(out, node->events);
}

static void do_vfs_clear_poll_events(int32_t pid, proto_t* in, proto_t* out) {
	PF->addi(out, -1);
	if(pid < 0 || pid >= _max_proc_table_num) {
		return;
	}
	uint32_t node_id = (uint32_t)proto_read_int(in);
	uint32_t events = (uint32_t)proto_read_int(in);
	vfs_node_t* node = vfs_get_node_by_id(node_id);
	if(node == NULL)
		return;
	node->events &= ~events;
	PF->addi(out, 0);
}

static void handle(int pid, int cmd, proto_t* in, proto_t* out, void* p) {
	(void)p;
	if(pid < 0)
		return;

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
	case VFS_BLOCK:
		do_vfs_block(pid, in);
		break;
	case VFS_WAKEUP:
		do_vfs_wakeup(pid, in);
		break;
	case VFS_GET_POLL_EVENTS:
		do_vfs_get_poll_events(pid, in, out);
		break;
	case VFS_CLEAR_POLL_EVENTS:
		do_vfs_clear_poll_events(pid, in, out);
		break;
	case VFS_SET_BY_FD:
		do_vfs_set_by_fd(pid, in, out);
		break;
	default:
		break;
	}
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
		usleep(10000);
		clear_zombies();
	}

	free(_proc_fds_table);
	return 0;
}
