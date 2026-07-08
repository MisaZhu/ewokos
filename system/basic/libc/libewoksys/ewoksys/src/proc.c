#include <ewoksys/proc.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vfs.h>
#include <ewoksys/syscall.h>
#include <ewoksys/sys.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ewoksys/core.h>
#include <malloc.h>
#include <fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif

bool _proc_global_need_lock = false;
static pthread_mutex_t _proc_global_lock = 0;
static int _reent_dep = 0;

int _vfsd_pid = -1;
int _cored_pid = -1;
static int _lock_thread = -1;

void proc_init(void) {
	_vfsd_pid = -1;
	_cored_pid = -1;
	_lock_thread = -1;
	_proc_global_need_lock = false;
	_reent_dep = 0;
	_proc_global_lock = 0;
}

void proc_exit(void) {
	if(_proc_global_lock != 0)
		pthread_mutex_destroy(&_proc_global_lock);
}

void proc_priority(uint32_t pid, uint32_t priority) {
	syscall2(SYS_PROC_PRIORITY, pid, priority);
}

void proc_malloc_lock_prepare(void) {
	/*
	 * Allocate the malloc-guard mutex eagerly while the process is still
	 * single-threaded (called from thread_create() right before the first
	 * child thread is spawned). Lazy-initializing it from proc_global_lock()
	 * races when the first two threads malloc at the same time: both observe
	 * _proc_global_lock == 0, each semaphore_alloc()s a different id, and the
	 * losing writer's semaphore is leaked while the two threads briefly guard
	 * the heap with different locks -> free-list corruption at startup.
	 */
	if(_proc_global_lock == 0)
		pthread_mutex_init(&_proc_global_lock, NULL);
}

static inline void proc_global_lock(void) {
	int tid = pthread_self();
	if(tid == _lock_thread) {
		_reent_dep++;
		return;
	}

	if(_proc_global_lock == 0)
		pthread_mutex_init(&_proc_global_lock, NULL);

	pthread_mutex_lock(&_proc_global_lock);
	_lock_thread = tid;
}

static inline void proc_global_unlock(void) {
	int tid = pthread_self();
	if(tid == _lock_thread) {
		if(_reent_dep > 0)  {
			_reent_dep--;
			return;
		}
	}

	/*
	 * Clear ownership BEFORE releasing the mutex.
	 *
	 * If _lock_thread were cleared AFTER pthread_mutex_unlock(), a thread
	 * blocked in proc_global_lock() could acquire the mutex in that gap and
	 * set _lock_thread to itself, and our trailing "_lock_thread = -1" would
	 * then wipe the new owner. That new owner's next nested malloc/free would
	 * read _lock_thread == -1, fail the "already mine" check, and call
	 * semaphore_enter() again on a semaphore it already holds -> self-deadlock
	 * (the netd worker hangs, IPC FS_CMD_POLL times out) while the malloc heap
	 * is left half-updated. Under concurrent load (multiple worker threads all
	 * growing proto_t buffers) this corrupts the free list and faults with a
	 * jump to an unmapped address. Clearing under the lock closes the window.
	 */
	_lock_thread = -1;
	pthread_mutex_unlock(&_proc_global_lock);
}

void __malloc_lock (struct _reent *reent) {
	if(!_proc_global_need_lock)
		return;
	proc_global_lock();
}

void __malloc_unlock (struct _reent *reent) {
	if(!_proc_global_need_lock)
		return;
	proc_global_unlock();
}

inline int get_vfsd_pid(void) {
	if(_vfsd_pid < 0)
		_vfsd_pid = ipc_serv_get(IPC_SERV_VFS);
	return _vfsd_pid;
}

inline int get_cored_pid(void) {
	if(_cored_pid < 0)
		_cored_pid = syscall0(SYS_CORE_PID);
	return _cored_pid;
}

inline int proc_getpid(int pid) {
	if(pid < 0) {
		if(_current_pid < 0)
		 	_current_pid = syscall1(SYS_GET_PID, (ewokos_addr_t)pid);
		return _current_pid;
	}	

	if(_current_pid == pid)
		return pid;
	
	if(pid >= MAX_PROC_NUM)
		return -1;

	while(true) {
		if(_vsyscall_info->proc_info[pid].uuid == 0)
			return -1;

		if(_vsyscall_info->proc_info[pid].type == TASK_TYPE_PROC)
			return pid;
		pid = _vsyscall_info->proc_info[pid].father_pid;
	}
	return -1;
}

int proc_getpid_or_raw(int pid) {
	int owner = proc_getpid(pid);
	if(owner < 0)
		owner = pid;
	return owner;
}

inline int proc_fork(void) {
	int ret = syscall0(SYS_FORK);
	if(ret == 0) {
		_current_pid = -1;
		proc_getpid(-1);
	}
	return ret;
}

inline int proc_info(int pid, procinfo_t* info) {
	return syscall2(SYS_GET_PROC, (ewokos_addr_t)pid, (ewokos_addr_t)info);
}

inline void proc_detach(void) {
	syscall0(SYS_DETACH);
}

inline void proc_block(void) {
	syscall1(SYS_BLOCK, 0);
}

/**
 * @brief block the current proc waiting on a specific VFS node token.
 *
 * A non-zero token scopes the block so only a wakeup for the same node (or a
 * generic tokenless wakeup) can release it. This prevents an event on an
 * unrelated node from spuriously waking this proc.
 */
inline void proc_block_by(uint32_t token) {
	syscall1(SYS_BLOCK, (ewokos_addr_t)token);
}

/**
 * @brief wakeup process by pid
 * 
 * @param pid wakeup blocked process pid
 */
inline void proc_wakeup(int32_t pid) {
	syscall2(SYS_WAKEUP, (ewokos_addr_t)pid, 0);
}

/**
 * @brief wakeup process by pid, scoped to a VFS node token.
 *
 * Only releases the target proc if it is blocked on this token or blocked
 * generically; a proc blocked on a different node is left untouched.
 */
inline void proc_wakeup_by(int32_t pid, uint32_t token) {
	syscall2(SYS_WAKEUP, (ewokos_addr_t)pid, (ewokos_addr_t)token);
}


inline void proc_exec_elf(const char* cmd_line, const char* elf, int32_t size) {
	if(syscall3(SYS_EXEC_ELF, (ewokos_addr_t)cmd_line, (ewokos_addr_t)elf, (ewokos_addr_t)size) != 0)
		exit(-1);
}

static void saveenv() {
	char buf[256] = {0};
	char ** env;
	extern char ** environ;
	env = environ;
	for (env; *env; ++env) {
		memset(buf, 0, sizeof(buf));
		char* key = buf;
		char* value = buf;
		for(int i= 0;i < sizeof(buf)-1; i++){
			char c = (*env)[i];
			if(c == '\0'){
				break;
			}
			else if(c == '='){
				buf[i] = '\0';
				value = &buf[i+1];
			}else{
				buf[i] = c;
			}
		}
		if(key[0] != 0){
			core_set_env(key, value);
		}
  }
}

static void close_on_exec_fds(void) {
	for(int fd = 0; fd < MAX_OPEN_FILE_PER_PROC; fd++) {
		int flags = vfs_get_fd_flags(fd);
		if(flags >= 0 && (flags & FD_CLOEXEC) != 0) {
			vfs_close(fd);
		}
	}
}

int proc_exec(const char *name) {
	char fpath[64];
	int sz = 0;
	const char *p = name;
	saveenv();
	memset(fpath, 0, sizeof(fpath));
	for(int i = 0; i < sizeof(fpath); i++){
		if(name[i] == '\0' || name[i] == ' ' || name[i] == '\t' || name[i] == '\n')
			break;
		fpath[i] = name[i];
	}
	void* buf = vfs_readfile(fpath, &sz);

	if(buf == NULL) {
		return -1;
	}
	close_on_exec_fds();
	proc_exec_elf(name, buf, sz);
	free(buf);
	return 0;
}

inline uint32_t proc_check_uuid(int32_t pid, uint32_t uuid) {
	if(pid < 0 || pid >= MAX_PROC_NUM)
		return 0;

	if(_vsyscall_info->proc_info[pid].uuid == uuid)
		return uuid;
	return 0;
}

inline uint32_t proc_get_uuid(int32_t pid) {
	if(pid < 0 || pid >= MAX_PROC_NUM)
		return 0;
	return _vsyscall_info->proc_info[pid].uuid;
}

inline void* proc_malloc_expand(int32_t size) {
	//klog("proc_malloc_expand %d, pid: %d\n", size, getpid());
	return (void*)syscall1(SYS_MALLOC_EXPAND, (ewokos_addr_t)size);
}

inline void* proc_malloc_free(void) {
	//klog("proc_free, pid: %d\n", getpid());
	syscall0(SYS_FREE);
}

inline uint32_t proc_malloc_size(void) {
	return syscall0(SYS_MALLOC_SIZE);
}

int proc_usleep(uint32_t usecs) {
	if(usecs == 0)
		syscall0(SYS_YIELD);
	else
		syscall1(SYS_USLEEP, (ewokos_addr_t)usecs);
	return 0;
}

inline void proc_yield(void) {
	syscall0(SYS_YIELD);
}

#ifdef __cplusplus
}
#endif
