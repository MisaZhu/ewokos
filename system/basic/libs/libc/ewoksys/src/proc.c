#include <ewoksys/proc.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vfs.h>
#include <ewoksys/syscall.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

static int _vfsd_pid;
static int _cored_pid;
static int _cpid;

void proc_init(void) {
	_vfsd_pid = -1;
	_cored_pid = -1;
	_cpid = -1;
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
		if(_cpid < 0) {
 	 		_cpid = syscall1(SYS_GET_PID, pid);
		}
		return _cpid;
	}
 	return syscall1(SYS_GET_PID, pid);
}

inline void proc_detach(void) {
	syscall0(SYS_DETACH);
}

inline int proc_ping(int pid) {
	return syscall1(SYS_IPC_PING, (int32_t)pid);
}

inline void proc_ready_ping(void) {
	syscall0(SYS_IPC_READY);
}

void proc_wait_ready(int pid) {
	while(1) {
		if(proc_ping(pid) == 0)
			break;
		usleep(100000);
	}
}

inline void proc_block(int by_pid, uint32_t evt) {
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

inline uint32_t proc_check_uuid(int32_t pid, uint32_t uuid) {
	uint32_t ret = syscall1(SYS_PROC_UUID, pid);
	if(ret == uuid)
		return ret;
	return 0;
}

inline uint32_t proc_get_uuid(int32_t pid) {
	return syscall1(SYS_PROC_UUID, pid);
}

#ifdef __cplusplus
}
#endif

