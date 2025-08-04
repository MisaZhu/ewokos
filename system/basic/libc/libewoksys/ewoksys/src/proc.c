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
		
	pthread_mutex_unlock(&_proc_global_lock);
	_lock_thread = -1;
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

inline void proc_block_by(int by_pid, uint32_t evt) {
	syscall2(SYS_BLOCK, (ewokos_addr_t)by_pid, (ewokos_addr_t)evt);
}

inline void proc_wakeup(uint32_t evt) {
	syscall2(SYS_WAKEUP, -1, (ewokos_addr_t)evt);
}

inline void proc_wakeup_pid(int pid, uint32_t evt) {
	syscall2(SYS_WAKEUP, (ewokos_addr_t)pid, (ewokos_addr_t)evt);
}

inline void proc_exec_elf(const char* cmd_line, const char* elf, int32_t size) {
	if(syscall3(SYS_EXEC_ELF, (ewokos_addr_t)cmd_line, (ewokos_addr_t)elf, (ewokos_addr_t)size) != 0)
		exit(-1);
}

static int ewok_set_env(const char* name, const char* value) {
	proto_t in, out;
	PF->init(&out);
	PF->init(&in)->adds(&in, name)->adds(&in, value);

	int res = ipc_call(get_cored_pid(), CORE_CMD_SET_ENV, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		if(proto_read_int(&out) != 0) {
			res = -1;
		}
	}
	else {
		res = -1;
	}
	PF->clear(&out);
	return res;
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
			ewok_set_env(key, value);
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
	//if(size <= 0)
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

#ifdef __cplusplus
}
#endif

