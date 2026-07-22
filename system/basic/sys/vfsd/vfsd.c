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
#include <ewoksys/shm_pipe.h>
#include <sys/shm.h>
#include <procinfo.h>
#include <sysinfo.h>

typedef struct vfs_node {
  struct vfs_node* father;
  struct vfs_node* first_kid; /*first child*/
  struct vfs_node* last_kid; /*last child*/
  struct vfs_node* next; /*next brother*/
  struct vfs_node* prev; /*prev brother*/
  void* data_ptr;
  shm_pipe_t* shm_ring;     /* shared-memory pipe ring (NULL if not allocated) */
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
  /*
   * Tracks whether this fd owns a driver-side reference that must be
   * returned with FS_CMD_CLOSE when the fd dies without an explicit
   * user-space close. Set by vfs_open() (open/accept create driver
   * state) and by do_vfs_proc_clone() (fork sends FS_CMD_DUP per
   * inherited fd); cleared by vfs_dup()/vfs_dup2() because same-process
   * duplication never notifies the driver.
   */
  uint32_t driver_ref;
  fsinfo_t fsinfo;
} file_t;

static vfs_node_t* _vfs_root = NULL;
static mount_t _vfs_mounts[FS_MOUNT_MAX];
static map_t  _nodes_hash = NULL;
static uint32_t _next_node_id = 1;

typedef struct {
	int32_t pid;
	uint32_t uuid;
	queue_item_t item;
	queue_t* queue;
	uint32_t node_id;
} wait_entry_t;

typedef struct {
	uint32_t uuid;
	uint32_t state;
	int32_t owner_pid;
	wait_entry_t read_waiter;
	wait_entry_t write_waiter;
	file_t fds[MAX_OPEN_FILE_PER_PROC];
} proc_fds_t;

typedef struct {
	int32_t pid;
	uint32_t uuid;
} zombie_task_t;

typedef struct {
	int32_t pid;
	int32_t owner_pid;
	int32_t fd;
	file_t file;
} driver_close_task_t;

static proc_fds_t* _proc_fds_table = NULL;
static uint32_t    _max_proc_table_num = 0;
static queue_t     _zombie_tasks;
static queue_t     _driver_close_tasks;

static uint32_t vfs_get_node_id(vfs_node_t* node);
static void vfs_track_task_slot(int32_t pid);
static void enqueue_waiter(queue_t* q, int32_t pid, uint32_t uuid, bool wr, uint32_t node_id);

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

	queue_init(&_zombie_tasks);
	queue_init(&_driver_close_tasks);
	_nodes_hash = hashmap_new(0);
	_vfs_root = vfs_new_node();
	strcpy(_vfs_root->fsinfo.name, "/");
}

static file_t* vfs_get_file(int32_t pid, int32_t fd) {
	if(pid < 0 || pid >= _max_proc_table_num || fd < 0 || fd >= MAX_OPEN_FILE_PER_PROC)
		return NULL;
	return &_proc_fds_table[pid].fds[fd];
}

#define VFSD_BACKUP_FD0 (MAX_OPEN_FILE_PER_PROC-3)
#define VFSD_BACKUP_FD1 (MAX_OPEN_FILE_PER_PROC-2)

static int32_t vfs_fd_owner_pid(int32_t pid) {
	int32_t owner = proc_getpid(pid);
	if(owner < 0)
		owner = pid;
	return owner;
}

static file_t* vfs_check_fd(int32_t pid, int32_t fd) {
	int32_t owner = vfs_fd_owner_pid(pid);
	file_t* f = vfs_get_file(owner, fd);
	if(f == NULL || f->node == NULL)
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
	pid = vfs_fd_owner_pid(pid);
	if(pid < 0 || pid >= _max_proc_table_num)
		return -1;
	int32_t i;
	for(i=3; i<MAX_OPEN_FILE_PER_PROC; i++) { //0, 1, 2 reserved for stdio in/out/err
		if(_proc_fds_table[pid].fds[i].node == 0)
			return i;
	}
	return -1;
}

static int32_t vfs_open(int32_t pid, vfs_node_t* node, int32_t flags) {
	int32_t owner = vfs_fd_owner_pid(pid);
	if(node == NULL)
		return -1;
	if(owner < 0 || owner >= _max_proc_table_num)
		return -1;
	_proc_fds_table[owner].owner_pid = owner;

	int32_t fd = get_free_fd(owner);
	if(fd < 0)
		return -1;

	_proc_fds_table[owner].fds[fd].node = node;
	_proc_fds_table[owner].fds[fd].flags = flags;
	_proc_fds_table[owner].fds[fd].driver_ref = 1;
	memcpy(&_proc_fds_table[owner].fds[fd].fsinfo, &node->fsinfo, sizeof(fsinfo_t));
	_proc_fds_table[owner].fds[fd].fsinfo.node = vfs_get_node_id(node);
	_proc_fds_table[owner].fds[fd].fsinfo.mount_pid = get_mount_pid(node);

	if((flags & O_TRUNC) != 0)
		node->fsinfo.stat.size = 0;

	node->refs++;
	if((flags & (O_WRONLY | O_RDWR)) != 0)
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
	(void)events;
	if(waiter == NULL)
		return;
	if(waiter->pid < 0 || waiter->pid >= _max_proc_table_num)
		return;
	if(proc_check_uuid(waiter->pid, waiter->uuid) == 0)
		return;
	proc_wakeup_by(waiter->pid, vfs_get_node_id(node));
}

static inline wait_entry_t* get_wait_entry(int32_t pid, bool wr) {
	if(pid < 0 || pid >= _max_proc_table_num)
		return NULL;
	if(wr)
		return &_proc_fds_table[pid].write_waiter;
	return &_proc_fds_table[pid].read_waiter;
}

static inline int32_t get_tracked_owner_pid(int32_t pid) {
	if(pid < 0 || pid >= _max_proc_table_num)
		return pid;
	int32_t owner = _proc_fds_table[pid].owner_pid;
	if(owner <= 0)
		owner = vfs_fd_owner_pid(pid);
	return owner;
}

static void wait_queue_detach(queue_t* q, wait_entry_t* waiter) {
	queue_item_t* it;
	bool linked;

	if(q == NULL || waiter == NULL)
		return;
	if(waiter->queue != q)
		return;

	it = &waiter->item;
	linked = (q->head == it) || (q->tail == it) ||
			(it->prev != NULL) || (it->next != NULL);
	if(!linked) {
		waiter->queue = NULL;
		waiter->node_id = 0;
		memset(it, 0, sizeof(queue_item_t));
		return;
	}

	if(it->next != NULL)
		it->next->prev = it->prev;
	if(it->prev != NULL)
		it->prev->next = it->next;

	if(q->head == it)
		q->head = it->next;
	if(q->tail == it)
		q->tail = it->prev;
	if(q->num > 0)
		q->num--;

	memset(it, 0, sizeof(queue_item_t));
	waiter->queue = NULL;
	waiter->node_id = 0;
}

static void wait_queue_remove_entry(wait_entry_t* waiter) {
	if(waiter == NULL || waiter->queue == NULL)
		return;
	wait_queue_detach(waiter->queue, waiter);
}

static wait_entry_t* wait_queue_pop(queue_t* q) {
	if(q == NULL || q->head == NULL)
		return NULL;

	wait_entry_t* waiter = (wait_entry_t*)q->head->data;
	if(waiter == NULL)
		return NULL;
	wait_queue_detach(q, waiter);
	return waiter;
}

static void wait_queue_push(queue_t* q, wait_entry_t* waiter, uint32_t node_id) {
	queue_item_t* it;

	if(q == NULL || waiter == NULL)
		return;

	if(waiter->queue == q && waiter->node_id == node_id)
		return;
	if(waiter->queue != NULL)
		wait_queue_remove_entry(waiter);

	it = &waiter->item;
	memset(it, 0, sizeof(queue_item_t));
	it->data = waiter;

	if(q->tail == NULL) {
		q->head = q->tail = it;
	}
	else {
		q->tail->next = it;
		it->prev = q->tail;
		q->tail = it;
	}
	q->num++;
	waiter->queue = q;
	waiter->node_id = node_id;
}

static void wakeup_wait_queue(queue_t* q, vfs_node_t* node, int32_t events) {
	if(q == NULL)
		return;

	/*
	 * Iterate the queue and wake each registered process WITHOUT removing
	 * it. Popping entries here creates a race on SMP: the wakeup can be
	 * consumed by an unrelated block (e.g. IPC-return wait with token=0)
	 * before the process reaches its intended proc_block_by(node). If the
	 * entry was already popped, the process ends up blocked with no waiter
	 * on any queue and no pending sticky wake – hung forever.
	 *
	 * Leaving entries in place is safe: the process always calls
	 * vfs_unblock() to remove itself once it observes the event, and
	 * wait_queue_push() short-circuits if the entry is already present.
	 */
	queue_item_t* it = q->head;
	while(it != NULL) {
		wait_entry_t* waiter = (wait_entry_t*)it->data;
		it = it->next;
		if(waiter != NULL)
			wakeup_proc(waiter, node, events);
	}
}

static bool queue_zombie_task_match(void* data, void* check_data) {
	if(data == NULL || check_data == NULL)
		return false;
	zombie_task_t* task = (zombie_task_t*)data;
	zombie_task_t* check = (zombie_task_t*)check_data;
	return task->pid == check->pid && task->uuid == check->uuid;
}

static void remove_zombie_task(int32_t pid, uint32_t uuid) {
	if(uuid == 0)
		return;

	zombie_task_t check;
	check.pid = pid;
	check.uuid = uuid;

	queue_item_t* it = _zombie_tasks.head;
	while(it != NULL) {
		queue_item_t* next = it->next;
		if(queue_zombie_task_match(it->data, &check)) {
			free(it->data);
			queue_remove(&_zombie_tasks, it);
		}
		it = next;
	}
}

static void vfs_remove_proc_waiters(int32_t pid, uint32_t uuid) {
	if(pid < 0 || pid >= _max_proc_table_num || uuid == 0)
		return;

	wait_entry_t* read_waiter = get_wait_entry(pid, false);
	wait_entry_t* write_waiter = get_wait_entry(pid, true);
	if(read_waiter != NULL && read_waiter->uuid == uuid)
		wait_queue_remove_entry(read_waiter);
	if(write_waiter != NULL && write_waiter->uuid == uuid)
		wait_queue_remove_entry(write_waiter);
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

	if(node->shm_ring != NULL) {
		/* shared-memory pipe: check ring buffer directly */
		if(shm_pipe_readable(node->shm_ring) > 0)
			events |= VFS_EVT_RD;
		if(shm_pipe_writable(node->shm_ring) > 0)
			events |= VFS_EVT_WR;
	} else {
		buffer_t* buffer = (buffer_t*)node->data_ptr;
		if(buffer != NULL) {
			int32_t rest = buffer->size - buffer->offset;
			if(rest > 0)
				events |= VFS_EVT_RD;
			if(rest < BUFFER_SIZE)
				events |= VFS_EVT_WR;
		}
	}
	node->events = events;
}

static void vfs_driver_close(int32_t pid, int32_t owner_pid, int32_t fd, file_t* file) {
	if(file == NULL)
		return;
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

	int32_t mount_pid = file->fsinfo.mount_pid;
	if(mount_pid <= 0 || file->fsinfo.node == 0)
		return;

	proto_t in;
	PF->format(&in, "i,i,m,i,i",
		fd, file->fsinfo.node, &file->fsinfo, sizeof(fsinfo_t), pid, owner_pid);
		ipc_call(mount_pid, FS_CMD_CLOSE, &in, NULL);	
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

	/*
	 * Same-process dup2() only needs the new fd to resolve to the same device
	 * state as an existing live fd in that process. Let the device-side cache
	 * lazily clone from the surviving source fd on first access instead of
	 * synchronously round-tripping through FS_CMD_DUP while we are still inside
	 * the VFS_DUP2 IPC handler.
	 */
	if(from_pid == dup_pid)
		return;

	proto_t in;
	PF->format(&in, "i,i,i,m,i,i",
		from_fd, dup_fd, vfs_get_node_id(node), &file->fsinfo, sizeof(fsinfo_t),
		from_pid, dup_pid);
	int32_t mount_pid = vfs_get_mount_pid(node);
	if(mount_pid > 0) {
		/*
		 * FS_CMD_DUP must keep device-side per-fd state in sync, but the
		 * fire-and-forget IPC path can wedge here during dup2(sock -> stdio).
		 * Force the driver dup through the regular reply path so vfsd does not
		 * stall before the device server even receives the request.
		 */
		proto_t out;
		PF->init(&out);
		ipc_call(mount_pid, FS_CMD_DUP, &in, &out);
		PF->clear(&out);
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
		int32_t unread = 0;

		if(node->shm_ring != NULL) {
			unread = shm_pipe_readable(node->shm_ring);
		} else {
			buffer_t* buffer = (buffer_t*)node->data_ptr;
			if(buffer != NULL) {
				unread = buffer->size - buffer->offset;
				if(unread < 0)
					unread = 0;
			}
		}

		bool expose_close = (read_refs == 0) || (node->refs_w == 0);
		if(expose_close) {
			node->events |= VFS_EVT_CLOSE;
			/* Set close flags in shared ring so userspace readers/writers
			 * can detect close without IPC to vfsd. */
			if(node->shm_ring != NULL) {
				if(node->refs_w == 0) {
					__atomic_store_n(&node->shm_ring->writer_closed, 1, __ATOMIC_RELEASE);
					/* Wake shm-path reader directly */
					int32_t rpid = __atomic_load_n(&node->shm_ring->reader_pid, __ATOMIC_RELAXED);
					if(rpid > 0)
						proc_wakeup_by(rpid, node->node_id);
				}
				if(read_refs == 0) {
					__atomic_store_n(&node->shm_ring->reader_closed, 1, __ATOMIC_RELEASE);
					/* Wake shm-path writer directly */
					int32_t wpid = __atomic_load_n(&node->shm_ring->writer_pid, __ATOMIC_RELAXED);
					if(wpid > 0)
						proc_wakeup_by(wpid, node->node_id);
				}
			}
		}
		else
			node->events &= ~VFS_EVT_CLOSE;
		sync_pipe_poll_events(node);

		if(node->refs <= 0) {
			if(node->fsinfo.name[0] == 0) { //no refs and not fifo pipe
				if(node->shm_ring != NULL) {
					shmdt(node->shm_ring);
					node->shm_ring = NULL;
				}
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
		if(expose_close)
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
	int32_t owner = vfs_fd_owner_pid(pid);
	if(pid < 0 || fd < 0 || fd >= MAX_OPEN_FILE_PER_PROC)
		return;
	if(owner < 0 || owner >= _max_proc_table_num)
		return;
	_proc_fds_table[owner].owner_pid = owner;

	file_t* f = vfs_get_file(owner, fd);
	if(f != NULL && f->node != NULL) {
		/*
		 * Do NOT send FS_CMD_CLOSE here — the user-space vfs_close() already
		 * sends FS_CMD_CLOSE directly to the device driver before calling
		 * VFS_CLOSE on vfsd. Adding another would double-decrement the
		 * driver's per-socket reference count, causing premature release.
		 *
		 * Device-close notification for process exit is handled exclusively
		 * in clear_zombie(), where there is no user-space path.
		 */
		proc_file_close(owner, fd, f);
		memset(f, 0, sizeof(file_t));
	}
}

static vfs_node_t* vfs_dup(int32_t pid, int32_t from, int32_t *ret) {
	int32_t owner = vfs_fd_owner_pid(pid);
	if(from < 0 || from > MAX_OPEN_FILE_PER_PROC)
		return NULL;
	if(owner < 0 || owner >= _max_proc_table_num)
		return NULL;
	int32_t to = get_free_fd(owner);
	if(to < 0)
		return NULL;

	file_t* f = vfs_check_fd(owner, from);
	if(f == NULL) 
		return NULL;

	file_t* f_to = vfs_get_file(owner, to);
	if(f_to == NULL)
		return NULL;

	memcpy(f_to, f, sizeof(file_t));
	/* Same-process dup() never sends FS_CMD_DUP, so the new fd owns no
	 * driver-side reference; the surviving source fd keeps it. */
	f_to->driver_ref = 0;
	f->node->refs++;
	if((f->flags & (O_WRONLY | O_RDWR)) != 0)
		f->node->refs_w++;
	*ret = to;
	return f_to->node;
}

static vfs_node_t* vfs_dup2(int32_t pid, int32_t from, int32_t to) {
	int32_t owner = vfs_fd_owner_pid(pid);
	file_t* f = vfs_check_fd(owner, from);
	if(f == NULL) 
		return NULL;

	if(from == to)
		return f->node;

	if(from < 0 || from > MAX_OPEN_FILE_PER_PROC ||
			to < 0 || to > MAX_OPEN_FILE_PER_PROC)
		return NULL;
	if(owner < 0 || owner >= _max_proc_table_num)
		return NULL;

	/*
	 * dup2 atomically replaces the target fd; user space never gets a
	 * chance to close() the overwritten file, so unlike the explicit
	 * vfs_close() path (where libc notifies the driver first), no
	 * FS_CMD_CLOSE reaches the driver for the old target. If it owned
	 * a driver-side reference (driver_ref=1), return it through the
	 * deferred close queue now — otherwise the driver's per-fd ref
	 * leaks. A telnet shell's pipeline child dup2()ing a pipe over its
	 * inherited socket fd 0 (driver_ref=1 from fork) leaked the netd
	 * connection ref, keeping the connection task alive after exit.
	 */
	file_t* f_old = vfs_get_file(owner, to);
	if(f_old != NULL && f_old->node != NULL) {
		uint32_t type = f_old->fsinfo.type & FS_TYPE_MASK;
		if(type != FS_TYPE_FILE &&
				type != FS_TYPE_DIR &&
				type != FS_TYPE_LINK &&
				f_old->driver_ref &&
				f_old->fsinfo.mount_pid > 0 &&
				f_old->fsinfo.node != 0) {
			driver_close_task_t* task =
				(driver_close_task_t*)malloc(sizeof(driver_close_task_t));
			if(task != NULL) {
				task->pid = owner;
				task->owner_pid = owner;
				task->fd = to;
				task->file = *f_old;
				queue_push(&_driver_close_tasks, task);
			}
		}
	}
	vfs_close(owner, to);
	file_t* f_to = vfs_get_file(owner, to);
	if(f_to == NULL)
		return NULL;

	memcpy(f_to, f, sizeof(file_t));
	/* Same-process dup2() never sends FS_CMD_DUP, so the new fd owns no
	 * driver-side reference; the surviving source fd keeps it. */
	f_to->driver_ref = 0;
	f->node->refs++;
	if((f->flags & (O_WRONLY | O_RDWR)) != 0)
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
	if(node == NULL) {
		return;
	}
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
		node->shm_ring = NULL;
		node->fsinfo.data = 0;
		/*
		 * A freshly created FIFO must advertise its true poll state
		 * immediately (empty buffer => VFS_EVT_WR). Without this a blocking
		 * writer/reader on the named pipe spins forever waiting for an edge
		 * that is only ever asserted from a later pipe read/write/close.
		 */
		sync_pipe_poll_events(node);
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

	/*
	 * Permission checks must match the requested access mode:
	 * - O_RDONLY: require read
	 * - O_WRONLY: require write only
	 * - O_RDWR:   require both
	 *
	 * The previous logic always required R_OK as well, which made pure
	 * write opens fail for non-root users on write-only targets.
	 */
	if((flags & O_WRONLY) == 0 &&
			vfs_check_access(pid, &node->fsinfo, R_OK) != 0) {
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
		int32_t owner = vfs_fd_owner_pid(pid);
		file_t* file = vfs_check_fd(pid, fdto);
		vfs_driver_dup(owner, fd, owner, fdto, file);
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
		int32_t owner = vfs_fd_owner_pid(pid);
		file_t* file = vfs_check_fd(pid, fdto);
		vfs_driver_dup(owner, fd, owner, fdto, file);
		PF->addi(out, fdto)->add(out, gen_file_fsinfo(file), sizeof(fsinfo_t));
	}
	else {
		PF->addi(out, -1);
	}
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

	/*
	 * Allocate a shared-memory ring buffer so reader/writer can transfer
	 * data directly without IPC to vfsd. Falls back to the old buffer_t
	 * path if shm allocation fails.
	 */
	int32_t shm_id = shmget(IPC_PRIVATE, SHM_PIPE_PAGE_SIZE, IPC_CREAT | 0666);
	if(shm_id > 0) {
		shm_pipe_t* ring = (shm_pipe_t*)shmat(shm_id, NULL, 0);
		if(ring != NULL) {
			uint32_t nid = vfs_get_node_id(node);
			shm_pipe_init(ring, nid, shm_id);
			node->shm_ring = ring;
			node->data_ptr = NULL;
			node->fsinfo.data = (uint32_t)shm_id;
		} else {
			/* shm map failed, fall back to buffer */
			buffer_t* buf = (buffer_t*)malloc(sizeof(buffer_t));
			memset(buf, 0, sizeof(buffer_t));
			node->data_ptr = buf;
			node->shm_ring = NULL;
			node->fsinfo.data = 0;
		}
	} else {
		/* shm alloc failed, fall back to buffer */
		buffer_t* buf = (buffer_t*)malloc(sizeof(buffer_t));
		memset(buf, 0, sizeof(buffer_t));
		node->data_ptr = buf;
		node->shm_ring = NULL;
		node->fsinfo.data = 0;
	}
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
			addi(out, shm_id > 0 ? shm_id : 0)->
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
	int32_t block = proto_read_int(in);
	vfs_node_t* node = vfs_get_node_by_id(node_id);
	if(size < 0 || data == NULL || node == NULL) {
		return;
	}

	uint32_t read_refs = (node->refs >= node->refs_w) ? (node->refs - node->refs_w) : 0;
	if(read_refs == 0) { // reader side closed
		node->events |= VFS_EVT_CLOSE;
		sync_pipe_poll_events(node);
		do_node_wakeup(node, VFS_EVT_CLOSE);
		return;
	}

	if(node->shm_ring != NULL) {
		/* Write to shared ring buffer (same store used by direct shm path) */
		int32_t n = shm_pipe_write(node->shm_ring, data, size);
		if(n > 0) {
			sync_pipe_poll_events(node);
			do_node_wakeup(node, VFS_EVT_RD);
			/* Also wake the shm-path reader directly (it bypasses wait queue) */
			int32_t rpid = __atomic_load_n(&node->shm_ring->reader_pid, __ATOMIC_RELAXED);
			if(rpid > 0)
				proc_wakeup_by(rpid, node_id);
			PF->clear(out)->addi(out, n);
			return;
		}
	} else {
		buffer_t* buffer = (buffer_t*)node->data_ptr;
		if(buffer == NULL) {
			sync_pipe_poll_events(node);
			return;
		}
		size = buffer_write(buffer, data, size);
		if(size > 0) {
			node->fsinfo.state |= FS_STATE_CHANGED;
			sync_pipe_poll_events(node);
			do_node_wakeup(node, VFS_EVT_RD);
			PF->clear(out)->addi(out, size);
			return;
		}
	}

	/*
	 * Buffer is full. If the caller requested blocking, register the
	 * process on the write wait queue atomically.
	 * Also store the pid in shm ring so the shm-path reader can wake us.
	 */
	if(block) {
		if(node->shm_ring != NULL)
			__atomic_store_n(&node->shm_ring->writer_pid, pid, __ATOMIC_RELAXED);
		vfs_track_task_slot(pid);
		uint32_t uuid = proc_get_uuid(pid);
		if(uuid != 0)
			enqueue_waiter(&node->write_wait_queue, pid, uuid, true, node_id);
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
	int32_t block = proto_read_int(in);
	if(node == NULL || size < 0)
		return;

	if(node->shm_ring != NULL) {
		/* Read from shared ring buffer (same store used by direct shm path) */
		int32_t avail = shm_pipe_readable(node->shm_ring);

		if(avail == 0 && node->refs_w == 0) {
			node->events |= VFS_EVT_CLOSE;
			sync_pipe_poll_events(node);
			do_node_wakeup(node, VFS_EVT_CLOSE);
			return;
		}

		if(avail > 0 && size > 0) {
			char tmp[SHM_PIPE_DATA_SIZE];
			int32_t n = shm_pipe_read(node->shm_ring, tmp, size < avail ? size : avail);
			if(n > 0) {
				PF->clear(out)->addi(out, n)->add(out, tmp, n);
				sync_pipe_poll_events(node);
				do_node_wakeup(node, VFS_EVT_WR);
				/* Also wake the shm-path writer directly (it bypasses wait queue) */
				int32_t wpid = __atomic_load_n(&node->shm_ring->writer_pid, __ATOMIC_RELAXED);
				if(wpid > 0)
					proc_wakeup_by(wpid, node_id);
				if(shm_pipe_readable(node->shm_ring) == 0 && node->refs_w == 0) {
					node->events |= VFS_EVT_CLOSE;
					do_node_wakeup(node, VFS_EVT_CLOSE);
				}
				return;
			}
		}
	} else {
		buffer_t* buffer = (buffer_t*)node->data_ptr;
		if(buffer == NULL) {
			sync_pipe_poll_events(node);
			return;
		}

		int32_t unread = buffer->size - buffer->offset;
		if(unread < 0)
			unread = 0;

		if(unread == 0 && node->refs_w == 0) {
			node->events |= VFS_EVT_CLOSE;
			sync_pipe_poll_events(node);
			do_node_wakeup(node, VFS_EVT_CLOSE);
			return;
		}

		if(unread > 0 && size > 0) {
			size = size < unread ? size : unread;
			if(size > 0) {
				PF->clear(out)->addi(out, size)->add(out, buffer->buffer + buffer->offset, size);
				buffer->offset += size;
				if(buffer->offset == buffer->size) {
					buffer->offset = 0;
					buffer->size = 0;
				}
				int32_t unread_after = buffer->size - buffer->offset;
				if(unread_after < 0)
					unread_after = 0;
				if(unread_after == 0 && node->refs_w == 0)
					node->events |= VFS_EVT_CLOSE;
				sync_pipe_poll_events(node);
				do_node_wakeup(node, VFS_EVT_WR);
				if(unread_after == 0 && node->refs_w == 0)
					do_node_wakeup(node, VFS_EVT_CLOSE);
				return;
			}
		}
	}

	/*
	 * Pipe is empty but writer is still alive. If the caller requested
	 * blocking, register the process on the read wait queue atomically.
	 * Also store the pid in shm ring so the shm-path writer can wake us.
	 */
	if(block) {
		if(node->shm_ring != NULL)
			__atomic_store_n(&node->shm_ring->reader_pid, pid, __ATOMIC_RELAXED);
		vfs_track_task_slot(pid);
		uint32_t uuid = proc_get_uuid(pid);
		if(uuid != 0)
			enqueue_waiter(&node->read_wait_queue, pid, uuid, false, node_id);
	}

	sync_pipe_poll_events(node);
	PF->clear(out)->addi(out, 0); //retry
}

static void clear_zombie(int32_t cpid);

static void vfs_proc_exit(int32_t cpid) {
	if(cpid < 0 || cpid >= _max_proc_table_num)
		return;
	if(_proc_fds_table[cpid].uuid == 0) {
		wait_queue_remove_entry(&_proc_fds_table[cpid].read_waiter);
		wait_queue_remove_entry(&_proc_fds_table[cpid].write_waiter);
		memset(&_proc_fds_table[cpid].read_waiter, 0, sizeof(wait_entry_t));
		memset(&_proc_fds_table[cpid].write_waiter, 0, sizeof(wait_entry_t));
		/*
		 * A zero uuid does NOT mean "nothing to clean": core forwards
		 * KEV_PROC_CREATED/EXIT through a polled kevent queue (~50ms), so a
		 * short-lived process (e.g. `ps` in a shell pipeline on fast
		 * hardware) can already be dead when VFS_PROC_CLONE finally runs.
		 * do_vfs_proc_clone() then still copies the parent's fds and bumps
		 * node refs/refs_w (and dups driver-side refs via FS_CMD_DUP), but
		 * records uuid=0. Skipping fd cleanup here leaks those references
		 * forever: a pipe write end never drops (its reader never sees EOF)
		 * and a driver's per-fd ref never returns (netd connection task
		 * never released). Reap the slot's fds like any other zombie;
		 * clear_zombie() also resets the slot to UNUSED.
		 */
		clear_zombie(cpid);
		return;
	}
	_proc_fds_table[cpid].state = ZOMBIE;
	if(_proc_fds_table[cpid].uuid != 0) {
		zombie_task_t check;
		check.pid = cpid;
		check.uuid = _proc_fds_table[cpid].uuid;
		if(queue_in(&_zombie_tasks, &check, queue_zombie_task_match) == NULL) {
			zombie_task_t* task = (zombie_task_t*)malloc(sizeof(zombie_task_t));
			*task = check;
			queue_push(&_zombie_tasks, task);
		}
	}
}

static void clear_zombie(int32_t cpid) {
	if(cpid < 0)
		return;
	vfs_remove_proc_waiters(cpid, _proc_fds_table[cpid].uuid);
	int32_t owner_pid = get_tracked_owner_pid(cpid);

	int32_t i;
	for(i=0; i<MAX_OPEN_FILE_PER_PROC; i++) {
		file_t *f = &_proc_fds_table[cpid].fds[i];
		if(f->node != NULL) {
			file_t closing = *f;
			memset(f, 0, sizeof(file_t));
			uint32_t type = closing.fsinfo.type & FS_TYPE_MASK;
			/*
			 * Queue exactly one FS_CMD_CLOSE per fd that owns a driver-side
			 * reference (see file_t.driver_ref). Fork sends FS_CMD_DUP per
			 * inherited fd, so a process can hold several ref-owning fds of
			 * the same node (e.g. socket stdin + original fd + backup fd in
			 * a telnet shell's command children); deduplicating per node
			 * under-closes and leaks the driver task. dup2-created fds own
			 * no ref and must not be closed here.
			 */
			if(type != FS_TYPE_FILE &&
					type != FS_TYPE_DIR &&
					type != FS_TYPE_LINK &&
					closing.driver_ref &&
					closing.fsinfo.mount_pid > 0 &&
					closing.fsinfo.node != 0) {
				driver_close_task_t* task =
					(driver_close_task_t*)malloc(sizeof(driver_close_task_t));
				if(task != NULL) {
					task->pid = cpid;
					task->owner_pid = owner_pid;
					task->fd = i;
					task->file = closing;
					queue_push(&_driver_close_tasks, task);
				}
			}
			proc_file_close(cpid, i, &closing);
			continue;
		}
		memset(f, 0, sizeof(file_t));
	}

	_proc_fds_table[cpid].state = UNUSED;
	_proc_fds_table[cpid].uuid = 0;
	_proc_fds_table[cpid].owner_pid = 0;
}

static void clear_pending_zombies(void* p) {
	(void)p;
	while(true) {
		zombie_task_t* task = (zombie_task_t*)queue_pop(&_zombie_tasks);
		if(task == NULL)
			return;
		if(task->pid >= 0 &&
				task->pid < _max_proc_table_num &&
				_proc_fds_table[task->pid].state == ZOMBIE &&
				_proc_fds_table[task->pid].uuid == task->uuid) {
			clear_zombie(task->pid);
			free(task);
			return;
		}
		free(task);
	}
}

static void vfs_reset_task_waiters(int32_t pid) {
	if(pid < 0 || pid >= _max_proc_table_num)
		return;
	wait_queue_remove_entry(&_proc_fds_table[pid].read_waiter);
	wait_queue_remove_entry(&_proc_fds_table[pid].write_waiter);
	memset(&_proc_fds_table[pid].read_waiter, 0, sizeof(wait_entry_t));
	memset(&_proc_fds_table[pid].write_waiter, 0, sizeof(wait_entry_t));
}

static void vfs_track_task_slot(int32_t pid) {
	uint32_t uuid;

	if(pid < 0 || pid >= _max_proc_table_num)
		return;
	uuid = proc_get_uuid(pid);
	if(uuid == 0)
		return;

	if((_proc_fds_table[pid].state == RUNNING ||
			_proc_fds_table[pid].state == ZOMBIE) &&
			_proc_fds_table[pid].uuid != 0 &&
			_proc_fds_table[pid].uuid != uuid) {
		uint32_t old_uuid = _proc_fds_table[pid].uuid;
		remove_zombie_task(pid, old_uuid);
		/*
		 * exec() reloads a process image in-place and the kernel assigns a fresh
		 * uuid to the same process pid. The process-owned fd table must survive
		 * that transition; only old poll/block waiters tied to the previous image
		 * should be dropped. Reusing a different pid slot (threads or real pid
		 * reuse after exit) still goes through the full zombie cleanup path.
		 */
		if(_proc_fds_table[pid].state == RUNNING && vfs_fd_owner_pid(pid) == pid) {
			vfs_remove_proc_waiters(pid, old_uuid);
			vfs_reset_task_waiters(pid);
			_proc_fds_table[pid].uuid = uuid;
			return;
		}
		_proc_fds_table[pid].state = ZOMBIE;
		clear_zombie(pid);
	}

	_proc_fds_table[pid].state = RUNNING;
	_proc_fds_table[pid].owner_pid = vfs_fd_owner_pid(pid);
	_proc_fds_table[pid].uuid = uuid;
}

static bool flush_driver_close_task(void) {
	driver_close_task_t* task = NULL;
	/*
	 * The close-task queue is shared with the IPC handler, but unlike proc/node
	 * state it only needs a tiny critical section to pop one immutable snapshot.
			 */
			ipc_disable();
	task = (driver_close_task_t*)queue_pop(&_driver_close_tasks);
			ipc_enable();
	if(task == NULL)
		return false;
	vfs_driver_close(task->pid, task->owner_pid, task->fd, &task->file);
	free(task);
	return true;
}

static void do_vfs_proc_clone(int32_t pid, proto_t* in) {
	(void)pid;
	int fpid = proto_read_int(in);
	int cpid = proto_read_int(in);

	if(fpid < 0 || fpid >= _max_proc_table_num ||
			cpid < 0 || cpid >= _max_proc_table_num)
		return;

	if(_proc_fds_table[cpid].state == RUNNING ||
			_proc_fds_table[cpid].state == ZOMBIE) {
		uint32_t old_uuid = _proc_fds_table[cpid].uuid;
		/*
		 * The child slot is about to be reused immediately. We cannot leave the
		 * previous occupant queued for later async reaping, otherwise its stale
		 * fds/refs survive into the reused slot and the delayed zombie task may
		 * race with the new child. Reap the old slot synchronously here and drop
		 * any queued zombie task for the same generation.
		 */
		_proc_fds_table[cpid].state = ZOMBIE;
		remove_zombie_task(cpid, old_uuid);
		clear_zombie(cpid);
	}

	_proc_fds_table[cpid].state = RUNNING;
	_proc_fds_table[cpid].owner_pid = vfs_fd_owner_pid(cpid);
	_proc_fds_table[cpid].uuid = proc_get_uuid(cpid);
	
	int32_t i;
	for(i=0; i<MAX_OPEN_FILE_PER_PROC; i++) {
		file_t *f = &_proc_fds_table[fpid].fds[i];
		vfs_node_t* node = f->node;
		if(node != NULL) {
			file_t* file = &_proc_fds_table[cpid].fds[i];
			memcpy(file, f, sizeof(file_t));
			node->refs++;
			if((f->flags & (O_WRONLY | O_RDWR)) != 0)
				node->refs_w++;
			/*
			 * Fork notifies the driver for EVERY inherited device fd
			 * (FS_CMD_DUP below), including fds that were dup2-created
			 * and carried no driver ref in the parent. The child copy
			 * therefore always owns a fresh driver-side reference.
			 */
			file->driver_ref = 1;
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

static void enqueue_waiter(queue_t* q, int32_t pid, uint32_t uuid, bool wr, uint32_t node_id) {
	wait_entry_t* waiter = get_wait_entry(pid, wr);
	if(q == NULL || waiter == NULL)
		return;

	/*
	 * VFS_BLOCK is on the shell hot path. Reuse the per-proc waiter instead of
	 * malloc()ing a fresh wait record plus queue node for every block/unblock
	 * round-trip; allocator churn here shows up as a visible IPC timeout.
	 */
	waiter->pid = pid;
	waiter->uuid = uuid;
	wait_queue_push(q, waiter, node_id);
}

static void do_vfs_block(int32_t pid, proto_t* in) {
	int node_id = proto_read_int(in);
	int events = proto_read_int(in);

	if(node_id == 0)
		return;
    vfs_node_t* node = vfs_get_node_by_id(node_id);
	if(node == NULL)
		return;

	if((node->events & (uint32_t)events) != 0) {
		return;
	}

	vfs_track_task_slot(pid);
	uint32_t uuid = proc_get_uuid(pid);
	if(uuid == 0)
		return;

	bool rd = (events & VFS_EVT_RD) != 0;
	bool wr = (events & VFS_EVT_WR) != 0;

	/*
	 * A poll()/select() waiter can watch a single fd for both read and
	 * write readiness. Read edges only wake the read queue and write edges
	 * only wake the write queue, so a dual-interest waiter must live on
	 * BOTH queues; registering on one alone strands the other edge and the
	 * poll blocks forever. A waiter interested only in close/err/nval (no
	 * RD/WR) is parked on the read queue since those events wake both.
	 */
	if(rd || (!rd && !wr))
		enqueue_waiter(&node->read_wait_queue, pid, uuid, false, (uint32_t)node_id);
	if(wr)
		enqueue_waiter(&node->write_wait_queue, pid, uuid, true, (uint32_t)node_id);
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

/*
 * Remove the caller's waiter from a single node's queues. A process must be
 * registered on a node ONLY while it is actually blocked waiting on it;
 * otherwise a stale entry lets an unrelated node event (e.g. tty keyboard RD)
 * spuriously wake a process now blocked elsewhere (e.g. a shell write), because
 * the kernel proc_wakeup() is unconditional and not tied to any node.
 */
static void do_vfs_unblock(int32_t pid, proto_t* in) {
	int node_id = proto_read_int(in);
	if(node_id == 0)
		return;
	vfs_node_t* node = vfs_get_node_by_id(node_id);
	if(node == NULL)
		return;

	uint32_t uuid = proc_get_uuid(pid);
	wait_entry_t* read_waiter;
	wait_entry_t* write_waiter;
	if(uuid == 0)
		return;
	read_waiter = get_wait_entry(pid, false);
	write_waiter = get_wait_entry(pid, true);
	if(read_waiter != NULL && read_waiter->uuid == uuid &&
			read_waiter->queue == &node->read_wait_queue) {
		wait_queue_remove_entry(read_waiter);
	}
	if(write_waiter != NULL && write_waiter->uuid == uuid &&
			write_waiter->queue == &node->write_wait_queue) {
		wait_queue_remove_entry(write_waiter);
	}
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
	case VFS_UNBLOCK:
		do_vfs_unblock(pid, in);
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
	ipc_serv_run(handle, clear_pending_zombies, NULL, IPC_DEFAULT);

	while(true) {
		if(!flush_driver_close_task())
		usleep(10000);
	}

	free(_proc_fds_table);
	return 0;
}
