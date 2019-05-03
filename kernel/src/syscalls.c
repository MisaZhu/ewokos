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
		
	strncpy(_current_proc->cmd, cmd, CMD_MAX);
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

static int32_t syscall_ipc_ready() {
	return ipc_ready();
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

static int32_t syscall_pf_open(int32_t arg0, int32_t arg1) {
	return kf_open((fs_info_t*)arg0, arg1);
}

static int32_t syscall_pf_close(int32_t arg0) {
	kf_close(arg0);
	return  0;
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

static int32_t syscall_pf_node_by_fd(int32_t arg0, int32_t arg1) {
	return kf_node_info_by_fd(arg0, (fs_info_t*)arg1);
}

static int32_t syscall_pf_node_by_addr(int32_t arg0, int32_t arg1) {
	return kf_node_info_by_addr((uint32_t)arg0, (fs_info_t*)arg1);
}

static int32_t syscall_pf_node_update(int32_t arg0, int32_t arg1) {
	return kf_node_info_update((uint32_t)arg0, (fs_info_t*)arg1);
}

static int32_t syscall_pf_get_ref(int32_t arg0, int32_t arg1) {
	return kf_get_ref(arg0, arg1);
}

static int32_t syscall_kserv_reg(int32_t arg0) {
	return kserv_reg((const char*)arg0);
}

static int32_t syscall_kserv_ready() {
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
	strncpy(pwd, _current_proc->pwd,
		arg1 < FULL_NAME_MAX ? arg1: FULL_NAME_MAX);
	return (int)pwd;
}

static int32_t syscall_set_cwd(int32_t arg0) {
	const char* pwd = (const char*)arg0;
	strncpy(_current_proc->pwd, pwd, FULL_NAME_MAX);
	return 0;
}

static int32_t syscall_get_cmd(int32_t arg0, int32_t arg1) {
	char* cmd = (char*)arg0;
	strncpy(cmd, _current_proc->cmd, arg1);
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

static int32_t (*const _syscallHandler[])() = {
	[SYSCALL_DEV_INFO] = syscall_dev_info,
	[SYSCALL_DEV_CHAR_READ] = syscall_dev_char_read,
	[SYSCALL_DEV_CHAR_WRITE] = syscall_dev_char_write,
	[SYSCALL_DEV_BLOCK_READ] = syscall_dev_block_read,
	[SYSCALL_DEV_BLOCK_READ_DONE] = syscall_dev_block_read_done,
	[SYSCALL_DEV_BLOCK_WRITE] = syscall_dev_block_write,
	[SYSCALL_DEV_BLOCK_WRITE_DONE] = syscall_dev_block_write_done,

	[SYSCALL_SHM_ALLOC] = syscall_shm_alloc,
	[SYSCALL_SHM_FREE] = syscall_shm_free,
	[SYSCALL_SHM_MAP] = syscall_shm_map,
	[SYSCALL_SHM_UNMAP] = syscall_shm_unmap,

	[SYSCALL_FORK] = syscall_fork,
	[SYSCALL_GETPID] = syscall_getpid,
	[SYSCALL_EXEC_ELF] = syscall_exec_elf,
	[SYSCALL_WAIT] = syscall_wait,
	[SYSCALL_EXIT] = syscall_exit,
	[SYSCALL_SLEEP_MSEC] = syscall_sleep_msec,

	[SYSCALL_GET_ENV] = syscall_get_env,
	[SYSCALL_GET_ENV_NAME] = syscall_get_env_name,
	[SYSCALL_GET_ENV_VALUE] = syscall_get_env_value,
	[SYSCALL_SET_ENV] = syscall_set_env,

	[SYSCALL_THREAD] = syscall_thread,

	[SYSCALL_PMALLOC] = syscall_pmalloc,
	[SYSCALL_PFREE] = syscall_pfree,

	[SYSCALL_GET_CMD] = syscall_get_cmd,

	[SYSCALL_IPC_OPEN] = syscall_ipc_open,
	[SYSCALL_IPC_CLOSE] = syscall_ipc_close,
	[SYSCALL_IPC_WRITE] = syscall_ipc_write,
	[SYSCALL_IPC_READY] = syscall_ipc_ready,
	[SYSCALL_IPC_READ] = syscall_ipc_read,
	[SYSCALL_IPC_RING] = syscall_ipc_ring,
	[SYSCALL_IPC_PEER] = syscall_ipc_peer,

	[SYSCALL_PFILE_GET_SEEK] = syscall_pf_get_seek,
	[SYSCALL_PFILE_SEEK] = syscall_pf_seek,
	[SYSCALL_PFILE_OPEN] = syscall_pf_open,
	[SYSCALL_PFILE_CLOSE] = syscall_pf_close,
	[SYSCALL_PFILE_NODE_BY_FD] = syscall_pf_node_by_fd,
	[SYSCALL_PFILE_NODE_BY_ADDR] = syscall_pf_node_by_addr,
	[SYSCALL_PFILE_NODE_UPDATE] = syscall_pf_node_update,
	[SYSCALL_PFILE_GET_REF] = syscall_pf_get_ref,

	[SYSCALL_KSERV_REG] = syscall_kserv_reg,
	[SYSCALL_KSERV_READY] = syscall_kserv_ready,
	[SYSCALL_KSERV_GET_BY_NAME] = syscall_kserv_get_by_name,
	[SYSCALL_KSERV_GET_BY_PID] = syscall_kserv_get_by_pid,

	[SYSCALL_GET_CWD] = syscall_get_cwd,
	[SYSCALL_SET_CWD] = syscall_set_cwd,

	[SYSCALL_SET_UID] = syscall_set_uid,
	[SYSCALL_GET_UID] = syscall_get_uid,

	[SYSCALL_SEMAPHORE_LOCK] = syscall_semaphore_lock,
	[SYSCALL_SEMAPHORE_UNLOCK] = syscall_semaphore_unlock,
	[SYSCALL_SEMAPHORE_INIT] = syscall_semaphore_init,
	[SYSCALL_SEMAPHORE_CLOSE] = syscall_semaphore_close,

	[SYSCALL_SYSTEM_CMD] = syscall_system_cmd
};

/* kernel side of system calls. */
int32_t handle_syscall(int32_t code, int32_t arg0, int32_t arg1, int32_t arg2) {
	return _syscallHandler[code](arg0, arg1, arg2);
}

