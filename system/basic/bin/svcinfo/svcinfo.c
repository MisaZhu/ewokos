#include <procinfo.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ewoksys/syscall.h>
#include <ewoksys/sys.h>
#include <string.h>

static inline const char* svc_name(int32_t code) {
	switch(code) {
	case SYS_NONE:
		return "none";
	case SYS_EXIT:
		return "exit";
	case SYS_SIGNAL_SETUP:
		return "signal_setup";
	case SYS_SIGNAL:
		return "signal";
	case SYS_SIGNAL_END:
		return "signal_end";
	case SYS_MALLOC_EXPAND:
		return "malloc_expand";
	case SYS_MALLOC_SIZE:
		return "malloc_size";
	case SYS_FREE:
		return "free";
	case SYS_GET_PID:
		return "get_pid";
	case SYS_GET_THREAD_ID:
		return "get_threadid";
	case SYS_USLEEP:
		return "usleep";
	case SYS_EXEC_ELF:
		return "exec_elf";
	case SYS_FORK:
		return "fork";
	case SYS_DETACH:
		return "detach";
	case SYS_WAIT_PID:
		return "wait_pid";
	case SYS_YIELD: 
		return "yield";
	case SYS_PROC_SET_UID: 
		return "set_uid";
	case SYS_PROC_GET_UID: 
		return "get_uid";
	case SYS_PROC_SET_GID: 
		return "set_gid";
	case SYS_PROC_GET_GID: 
		return "get_gid";
	case SYS_PROC_GET_CMD: 
		return "get_cmd";
	case SYS_PROC_SET_CMD: 
		return "set_cmd";
	case SYS_GET_SYS_INFO:
		return "get_sys_info";
	case SYS_GET_SYS_STATE:
		return "get_sys_state";
	case SYS_GET_VSYSCALL_INFO:
		return "get_vsyscall_info";
	case SYS_GET_PROCS_NUM: 
		return "get_procs_num";
	case SYS_GET_PROCS: 
		return "get_procs";
	case SYS_GET_PROC: 
		return "get_proc";
	case SYS_PROC_SHM_GET:
		return "shm_alloc";
	case SYS_PROC_SHM_MAP:
		return "shm_map";
	case SYS_PROC_SHM_UNMAP:
		return "shm_unmap";
	case SYS_THREAD:
		return "thread";
	case SYS_KPRINT:
		return "kprintf";
	case SYS_MEM_MAP:
		return "mem_map";
	case SYS_IPC_SETUP:
		return "ipc_setup";
	case SYS_IPC_CALL:
		return "ipc_call";
	case SYS_IPC_GET_RETURN:
		return "ipc_get_return";
	case SYS_IPC_SET_RETURN:
		return "ipc_set_return";
	case SYS_IPC_END:
		return "ipc_end";
	case SYS_IPC_GET_ARG:
		return "ipc_get_arg";
	case SYS_IPC_PING:
		return "ipc_ping";
	case SYS_IPC_READY:
		return "ipc_ready";
	case SYS_GET_KEVENT:
		return "get_kevent";
	case SYS_WAKEUP:
		return "wakeup";
	case SYS_BLOCK:
		return "block";
	case SYS_CORE_READY:
		return "core_ready";
	case SYS_CORE_PID:
		return "core_pid";
	case SYS_IPC_DISABLE:
		return "ipc_disable";
	case SYS_IPC_ENABLE:
		return "ipc_enable";
	case SYS_INTR_SETUP:
		return "intr_setup";
	case SYS_INTR_END:
		return "intr_end";
	case SYS_SEMAPHORE_ALLOC:
		return "semaphore_alloc";
	case SYS_SEMAPHORE_FREE:
		return "semaphore_free";
	case SYS_SEMAPHORE_ENTER:
		return "semaphore_enter";
	case SYS_SEMAPHORE_QUIT:
		return "semaphore_quit";
	case SYS_SOFT_INT:
		return "soft_int";
	case SYS_V2P:
		return "sys_v2p";
	case SYS_P2V:
		return "sys_p2v";
	}
	return "unknown";
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	sys_state_t sys_state;

	syscall1(SYS_GET_SYS_STATE, (ewokos_addr_t)&sys_state);
	printf("SVC  TIMES      %%  TYPE\n"
				 "----------------------------------\n");
	for(int i=0; i<SYS_CALL_NUM; i++) {
		printf("%-4d %-8d %2d%%  %s\n", i, 
				sys_state.svc_counter[i],
				sys_state.svc_counter[i]*100/sys_state.svc_total,
				svc_name(i));
	}
	printf("\ntotal:  %d\n", sys_state.svc_total);
	return 0;
}
