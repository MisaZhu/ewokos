#include <sys/proc.h>
#include <sys/ipc.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif


int get_procd_pid(void) {
	return ipc_serv_get(IPC_SERV_PROC);
}

int proc_getpid(int pid) {
  return syscall1(SYS_GET_PID, pid);
}

void proc_detach(void) {
	syscall0(SYS_DETACH);
}

int proc_ping(int pid) {
	return syscall1(SYS_PROC_PING, (int32_t)pid);
}

void proc_ready_ping(void) {
	syscall0(SYS_PROC_READY_PING);
}

void proc_wait_ready(int pid) {
	while(1) {
		if(proc_ping(pid) == 0)
			break;
		usleep(10000);
	}
}

void proc_block(int by_pid, uint32_t evt) {
	syscall2(SYS_BLOCK, by_pid, evt);
}

void proc_wakeup(uint32_t evt) {
	syscall1(SYS_WAKEUP, evt);
}

void proc_exec_elf(const char* cmd_line, const char* elf, int32_t size) {
	syscall3(SYS_EXEC_ELF, (int32_t)cmd_line, (int32_t)elf, size);
}

#ifdef __cplusplus
}
#endif

