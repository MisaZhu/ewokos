#include <ewoksys/proc.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vfs.h>
#include <ewoksys/syscall.h>
#include <pthread.h>
#include <unistd.h>
#include <malloc.h>

#ifdef __cplusplus
extern "C" {
#endif

bool _proc_global_need_lock = false;
static pthread_mutex_t _proc_global_lock;
static int _reent_dep = 0;

static int _vfsd_pid;
static int _cored_pid;
static int _lock_thread;

void proc_init(void) {
	_vfsd_pid = -1;
	_cored_pid = -1;
	_lock_thread = -1;
	_proc_global_need_lock = false;
	_reent_dep = 0;
	pthread_mutex_init(&_proc_global_lock, NULL);
}

void proc_exit(void) {
	pthread_mutex_destroy(&_proc_global_lock);
}


void proc_global_lock(void) {
	if(!_proc_global_need_lock)
		return -1;
	int tid = pthread_self();
	if(tid == _lock_thread) {
		_reent_dep++;
		return;
	}

	pthread_mutex_lock(&_proc_global_lock);
	_lock_thread = tid;
}

void proc_global_unlock(void) {
	if(!_proc_global_need_lock)
		return;

	if(_reent_dep > 0)  {
		_reent_dep--;
		return;
	}
		
	pthread_mutex_unlock(&_proc_global_lock);
	_lock_thread = -1;
}

void __malloc_lock (struct _reent *reent) {
	proc_global_lock();
}

void __malloc_unlock (struct _reent *reent) {
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
 	return syscall1(SYS_GET_PID, pid);
}

inline int proc_info(int pid, procinfo_t* info) {
	return syscall2(SYS_GET_PROC, pid, (int32_t)info);
}

inline void proc_detach(void) {
	syscall0(SYS_DETACH);
}

inline void proc_block_by(int by_pid, uint32_t evt) {
	syscall2(SYS_BLOCK, by_pid, evt);
}

inline void proc_wakeup(uint32_t evt) {
	syscall2(SYS_WAKEUP, -1, evt);
}

inline void proc_wakeup_pid(int pid, uint32_t evt) {
	syscall2(SYS_WAKEUP, pid, evt);
}

inline void proc_exec_elf(const char* cmd_line, const char* elf, int32_t size) {
	syscall3(SYS_EXEC_ELF, (int32_t)cmd_line, (int32_t)elf, size);
}

int  proc_exec(const char* cmd_line) {
	char* arg0[] = {cmd_line, NULL};
	char* arg1[] = {"", NULL};
	return execve(cmd_line, arg0, arg1); 		
}

inline uint32_t proc_check_uuid(int32_t pid, uint32_t uuid) {
	uint32_t ret = syscall1(SYS_PROC_UUID, pid);
	if(ret == uuid)
		return ret;
	return 0;
}

inline uint32_t proc_get_uuid(int32_t pid) {
	return syscall1(SYS_PROC_UUID, pid);
}

inline void* proc_malloc_expand(uint32_t size) {
	return (void*)syscall1(SYS_MALLOC_EXPAND, (int32_t)size);
}

inline void* proc_malloc_free(void) {
	syscall0(SYS_FREE);
}

inline uint32_t proc_malloc_size(void) {
	return syscall0(SYS_MALLOC_SIZE);
}

int proc_usleep(uint32_t usecs) {
	if(usecs == 0)
		syscall0(SYS_YIELD);
	else
		syscall1(SYS_USLEEP, usecs);
	return 0;
}

#ifdef __cplusplus
}
#endif

