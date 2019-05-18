#include <syscallcode.h>
#include <system.h>
#include <syscalls.h>
#include <types.h>
#include <proc.h>
#include <kernel.h>
#include <kstring.h>
#include <mm/trunkmalloc.h>
#include <mm/kmalloc.h>
#include <mm/shm.h>
#include <ipc.h>
#include <types.h>
#include <kserv.h>
#include <fsinfo.h>
#include <scheduler.h>
#include <hardware.h>
#include <semaphore.h>
#include <dev/basic_dev.h>

static int32_t syscall_dev_info(int32_t arg0, int32_t arg1) {
	return dev_info(arg0, (void*)arg1);
}

static int32_t syscall_dev_char_read(int32_t arg0, int32_t arg1, int32_t arg2) {
	return dev_char_read(arg0, (void*)arg1, (uint32_t)arg2);
}

static int32_t syscall_dev_char_write(int32_t arg0, int32_t arg1, int32_t arg2) {
	return dev_char_write(arg0, (void*)arg1, (uint32_t)arg2);
}

static int32_t syscall_dev_block_read(int32_t arg0, int32_t arg1) {
	return dev_block_read(arg0, arg1);
}

static int32_t syscall_dev_block_read_done(int32_t arg0, int32_t arg1) {
	return dev_block_read_done(arg0, (void*)arg1);
}

static int32_t syscall_dev_block_write(int32_t arg0, int32_t arg1, int32_t arg2) {
	return dev_block_write(arg0, (uint32_t)arg1, (void*)arg2);
}

static int32_t syscall_dev_block_write_done(int32_t arg0) {
	return dev_block_write_done(arg0);
}

static int32_t syscall_shm_alloc(int arg0) {
	return shm_alloc((uint32_t)arg0);
}

static int32_t syscall_shm_map(int arg0) {
	return (int32_t)shm_proc_map(_current_proc->pid, arg0);
}

static int32_t syscall_shm_free(int arg0) {
	shm_free(arg0);
	return 0;
}

static int32_t syscall_shm_unmap(int arg0) {
	return shm_proc_unmap(_current_proc->pid, arg0);
}

static int32_t syscall_exec_elf(int32_t arg0, int32_t arg1, int32_t arg2) {
	const char*cmd = (const char*)arg0;
	const char*p = (const char*)arg1;
	if(p == NULL || cmd == NULL)
		return -1;
		
	tstr_cpy(_current_proc->cmd, cmd);
	if(!proc_load(_current_proc, p, (uint32_t)arg2)) {
		return -1;
	}
	proc_start(_current_proc);
	return 0;
}

static int32_t syscall_fork(void) {
	process_t* proc = kfork(TYPE_PROC);
	if(proc == NULL)
		return -1;
	return proc->pid;
}

static int32_t syscall_getpid(void) {
	return _current_proc->pid;
}

static int32_t syscall_exit(int32_t arg0) {
	(void)arg0;
	proc_exit(_current_proc);
	return 0;
}

static int32_t syscall_sleep_msec(int32_t arg0) {
	proc_sleep_msec((uint32_t)arg0);
	return 0;
}

static int32_t syscall_get_env(int32_t arg0, int32_t arg1, int32_t arg2) {
	const char* v = proc_get_env((const char*)arg0);
	if(v == NULL)
		((char*)arg1)[0] = 0;
	else
		strncpy((char*)arg1, v, arg2);
	return 0;
}

static int32_t syscall_get_env_name(int32_t arg0, int32_t arg1, int32_t arg2) {
	const char* n = proc_get_env_name(arg0);
	if(n[0] == 0)
		return -1;
	strncpy((char*)arg1, n, arg2);
	return 0;
}

static int32_t syscall_get_env_value(int32_t arg0, int32_t arg1, int32_t arg2) {
	const char* v = proc_get_env_value(arg0);
	if(v == NULL)
		((char*)arg1)[0] = 0;
	else
		strncpy((char*)arg1, v, arg2);
	return 0;
}

static int32_t syscall_set_env(int32_t arg0, int32_t arg1) {
	return proc_set_env((const char*)arg0, (const char*)arg1);
}

static int32_t syscall_thread(int32_t arg0, int32_t arg1) {
	process_t* proc = kfork(TYPE_THREAD);
	if(proc == NULL)
		return -1;
	proc->context[RESTART_ADDR] = (uint32_t)arg0;
	proc->context[R0] = (uint32_t)arg1;
	return proc->pid;
}

static int32_t syscall_wait(int32_t arg0) {
	return proc_wait(arg0);
}

static int32_t syscall_pmalloc(int32_t arg0) {
	char* p = (char*)pmalloc((uint32_t)arg0);
	return (int)p;
}

static int32_t syscall_pfree(int32_t arg0) {
	char* p = (char*)arg0;
	pfree(p);
	return 0;
}

static int32_t syscall_ipc_open(int32_t arg0, int32_t arg1) {
	return ipc_open(arg0, arg1);
}

static int32_t syscall_ipc_ready(int32_t arg0) {
	return ipc_ready((bool)arg0);
}

static int32_t syscall_ipc_close(int32_t arg0) {
	return ipc_close(arg0);
}

static int32_t syscall_ipc_write(int32_t arg0, int32_t arg1, int32_t arg2) {
	return ipc_write(arg0, (void*)arg1, arg2);
}

static int32_t syscall_ipc_read(int32_t arg0, int32_t arg1, int32_t arg2) {
	return ipc_read(arg0, (void*)arg1, arg2);
}

static int32_t syscall_ipc_ring(int32_t arg0) {
	return ipc_ring(arg0);
}

static int32_t syscall_ipc_peer(int32_t arg0) {
	return ipc_peer(arg0);
}

static int32_t syscall_pf_open(int32_t arg0, int32_t arg1, int32_t arg2) {
	if(_current_proc->owner > 0)
		return -1;
	return kf_open(arg0, (fs_info_t*)arg1, arg2);
}

static int32_t syscall_pf_close(int32_t arg0, int32_t arg1) {
	if(_current_proc->owner > 0)
		return -1;
	kf_close(arg0, arg1);
	return  0;
}

static int32_t syscall_pf_dup2(int32_t arg0, int32_t arg1) {
	return kf_dup2(arg0, arg1);
}

static int32_t syscall_pf_serv_pid_by_fd(int32_t arg0) {
	fs_info_t info;
	if(kf_node_info_by_fd(_current_proc->pid, arg0, &info) < 0)
		return -1;
	return info.dev_serv_pid;
}

static int32_t syscall_pf_seek(int32_t arg0, int32_t arg1) {
	process_t* proc = _current_proc;
	if(proc == NULL)
		return -1;

	int32_t fd = arg0;
	if(fd < 0 || fd >= FILE_MAX)
		return -1;

	kfile_t* kf = proc->space->files[fd].kf;
	if(kf == NULL || kf->node_info.node == 0)
		return -1;
	proc->space->files[fd].seek = arg1;
	return arg1;
}

static int32_t syscall_pf_get_seek(int32_t arg0) {
	process_t* proc = _current_proc;
	if(proc == NULL)
		return -1;

	int32_t fd = arg0;
	if(fd < 0 || fd >= FILE_MAX)
		return -1;

	kfile_t* kf = proc->space->files[fd].kf;
	if(kf == NULL || kf->node_info.node == 0)
		return -1;
	return proc->space->files[fd].seek;
}

static int32_t syscall_pf_node_by_pid_fd(int32_t arg0, int32_t arg1, int32_t arg2) {
	if(_current_proc->owner > 0)
		return -1;
	return kf_node_info_by_fd(arg0, arg1, (fs_info_t*)arg2);
}

static int32_t syscall_pf_node_update(int32_t arg0) {
	if(_current_proc->owner > 0)
		return -1;
	return kf_node_update((fs_info_t*)arg0);
}

static int32_t syscall_pf_get_ref(int32_t arg0, int32_t arg1) {
	if(_current_proc->owner > 0)
		return -1;
	return kf_get_ref(arg0, arg1);
}

static int32_t syscall_kserv_reg(int32_t arg0) {
	if(_current_proc->owner > 0)
		return -1;
	return kserv_reg((const char*)arg0);
}

static int32_t syscall_kserv_ready(void) {
	if(_current_proc->owner > 0)
		return -1;
	return kserv_ready();
}

static int32_t syscall_kserv_get_by_name(int32_t arg0) {
	return kserv_get_by_name((const char*)arg0);
}

static int32_t syscall_kserv_get_by_pid(int32_t arg0) {
	return kserv_get_by_pid(arg0);
}

static int32_t syscall_get_cwd(int32_t arg0, int32_t arg1) {
	char* pwd = (char*)arg0;
	strncpy(pwd, CS(_current_proc->pwd), arg1);
	return (int)pwd;
}

static int32_t syscall_set_cwd(int32_t arg0) {
	const char* pwd = (const char*)arg0;
	tstr_cpy(_current_proc->pwd, pwd);
	return 0;
}

static int32_t syscall_get_cmd(int32_t arg0, int32_t arg1, int32_t arg2) {
	process_t* proc = proc_get(arg0);
	if(proc == NULL)
		return -1;
	char* cmd = (char*)arg1;
	strncpy(cmd, CS(proc->cmd), arg2);
	return arg0;
}

static int32_t syscall_set_uid(int32_t arg0, int32_t arg1) {
	if(arg1 < 0 || _current_proc->owner > 0) {/*current process not kernel proc*/
		return -1;
	}
	
	process_t* proc = proc_get(arg0);
	if(proc == NULL) {
		return -1;
	}

	proc->owner = arg1;
	return 0;
}

static int32_t syscall_get_uid(int32_t arg0) {
	process_t* proc = proc_get(arg0);
	if(proc == NULL)
		return -1;
	return proc->owner;
}

static int32_t syscall_semaphore_init(int32_t arg0) {
	return semaphore_init((int32_t*)arg0);
}

static int32_t syscall_semaphore_close(int32_t arg0) {
	return semaphore_close((int32_t*)arg0);
}

static int32_t syscall_semaphore_lock(int32_t arg0) {
	return semaphore_lock((int32_t*)arg0);
}

static int32_t syscall_semaphore_unlock(int32_t arg0) {
	return semaphore_unlock((int32_t*)arg0);
}

static int32_t syscall_system_cmd(int32_t arg0, int32_t arg1, int32_t arg2) { 
	//arg0: command id; arg1,arg2: command args
	return system_cmd(arg0, arg1, arg2);
}

/* kernel side of system calls. */
int32_t handle_syscall(int32_t code, int32_t arg0, int32_t arg1, int32_t arg2) {
	switch(code) {
		case SYSCALL_DEV_INFO: return  syscall_dev_info(arg0, arg1);
		case SYSCALL_DEV_CHAR_READ: return  syscall_dev_char_read(arg0, arg1, arg2);
		case SYSCALL_DEV_CHAR_WRITE: return  syscall_dev_char_write(arg0, arg1, arg2);
		case SYSCALL_DEV_BLOCK_READ: return  syscall_dev_block_read(arg0, arg1);
		case SYSCALL_DEV_BLOCK_READ_DONE: return  syscall_dev_block_read_done(arg0, arg1);
		case SYSCALL_DEV_BLOCK_WRITE: return  syscall_dev_block_write(arg0, arg1, arg2);
		case SYSCALL_DEV_BLOCK_WRITE_DONE: return  syscall_dev_block_write_done(arg0);

		case SYSCALL_SHM_ALLOC: return  syscall_shm_alloc(arg0);
		case SYSCALL_SHM_FREE: return  syscall_shm_free(arg0);
		case SYSCALL_SHM_MAP: return  syscall_shm_map(arg0);
		case SYSCALL_SHM_UNMAP: return  syscall_shm_unmap(arg0);

		case SYSCALL_FORK: return  syscall_fork();
		case SYSCALL_GETPID: return  syscall_getpid();
		case SYSCALL_EXEC_ELF: return  syscall_exec_elf(arg0, arg1, arg2);
		case SYSCALL_WAIT: return  syscall_wait(arg0);
		case SYSCALL_EXIT: return  syscall_exit(arg0);
		case SYSCALL_SLEEP_MSEC: return  syscall_sleep_msec(arg0);
		case SYSCALL_THREAD: return  syscall_thread(arg0, arg1);

		case SYSCALL_GET_ENV: return  syscall_get_env(arg0, arg1, arg2);
		case SYSCALL_GET_ENV_NAME: return  syscall_get_env_name(arg0, arg1, arg2);
		case SYSCALL_GET_ENV_VALUE: return  syscall_get_env_value(arg0, arg1, arg2);
		case SYSCALL_SET_ENV: return  syscall_set_env(arg0, arg1);

		case SYSCALL_PMALLOC: return  syscall_pmalloc(arg0);
		case SYSCALL_PFREE: return  syscall_pfree(arg0);

		case SYSCALL_IPC_OPEN: return  syscall_ipc_open(arg0, arg1);
		case SYSCALL_IPC_CLOSE: return  syscall_ipc_close(arg0);
		case SYSCALL_IPC_WRITE: return  syscall_ipc_write(arg0, arg1, arg2);
		case SYSCALL_IPC_READY: return  syscall_ipc_ready(arg0);
		case SYSCALL_IPC_READ: return  syscall_ipc_read(arg0, arg1, arg2);
		case SYSCALL_IPC_RING: return  syscall_ipc_ring(arg0);
		case SYSCALL_IPC_PEER: return  syscall_ipc_peer(arg0);

		case SYSCALL_PFILE_GET_SEEK: return  syscall_pf_get_seek(arg0);
		case SYSCALL_PFILE_SEEK: return  syscall_pf_seek(arg0, arg1);
		case SYSCALL_PFILE_OPEN: return  syscall_pf_open(arg0, arg1, arg2);
		case SYSCALL_PFILE_CLOSE: return  syscall_pf_close(arg0, arg1);
		case SYSCALL_PFILE_DUP2: return  syscall_pf_dup2(arg0, arg1);
		case SYSCALL_PFILE_SERV_PID_BY_FD: return  syscall_pf_serv_pid_by_fd(arg0);
		case SYSCALL_PFILE_INFO_BY_PID_FD: return  syscall_pf_node_by_pid_fd(arg0, arg1, arg2);
		case SYSCALL_PFILE_INFO_UPDATE: return  syscall_pf_node_update(arg0);
		case SYSCALL_PFILE_GET_REF: return  syscall_pf_get_ref(arg0, arg1);

		case SYSCALL_KSERV_REG: return  syscall_kserv_reg(arg0);
		case SYSCALL_KSERV_READY: return  syscall_kserv_ready();
		case SYSCALL_KSERV_GET_BY_NAME: return  syscall_kserv_get_by_name(arg0);
		case SYSCALL_KSERV_GET_BY_PID: return  syscall_kserv_get_by_pid(arg0);

		case SYSCALL_GET_CMD: return  syscall_get_cmd(arg0, arg1, arg2);
		case SYSCALL_GET_CWD: return  syscall_get_cwd(arg0, arg1);
		case SYSCALL_SET_CWD: return  syscall_set_cwd(arg0);

		case SYSCALL_SET_UID: return  syscall_set_uid(arg0, arg1);
		case SYSCALL_GET_UID: return  syscall_get_uid(arg0);

		case SYSCALL_SEMAPHORE_LOCK: return  syscall_semaphore_lock(arg0);
		case SYSCALL_SEMAPHORE_UNLOCK: return  syscall_semaphore_unlock(arg0);
		case SYSCALL_SEMAPHORE_INIT: return  syscall_semaphore_init(arg0);
		case SYSCALL_SEMAPHORE_CLOSE: return  syscall_semaphore_close(arg0);

		case SYSCALL_SYSTEM_CMD: return  syscall_system_cmd(arg0, arg1, arg2);
	}
	return -1;
}

