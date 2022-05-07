#include <sys/proc.h>
#include <sys/ipc.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

static int _vfsd_pid;

void proc_init(void) {
	_vfsd_pid = -1;
}

inline int get_vfsd_pid(void) {
	if(_vfsd_pid < 0)
		_vfsd_pid = ipc_serv_get(IPC_SERV_VFS);
	return _vfsd_pid;
}

inline int proc_getpid(int pid) {
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
	syscall1(SYS_WAKEUP, evt);
}

inline void proc_exec_elf(const char* cmd_line, const char* elf, int32_t size) {
	syscall3(SYS_EXEC_ELF, (int32_t)cmd_line, (int32_t)elf, size);
}

#ifdef __cplusplus
}
#endif

