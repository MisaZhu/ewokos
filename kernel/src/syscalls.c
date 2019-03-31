#include <syscallcode.h>
#include <system.h>
#include <syscalls.h>
#include <types.h>
#include <dev/uart.h>
#include <proc.h>
#include <kernel.h>
#include <kstring.h>
#include <mm/trunkmalloc.h>
#include <mm/kmalloc.h>
#include <mm/shm.h>
#include <kipc.h>
#include <types.h>
#include <kserv.h>
#include <fsinfo.h>
#include <scheduler.h>
#include <hardware.h>
#include <semaphore.h>
#include <dev/initrd.h>
#include <dev/fb.h>

static int32_t syscall_uart_putch(int32_t c) {
	uart_putch(c);
	return 0;
}

static int32_t syscall_uart_getch() {
	int32_t r = uart_getch();
	return r;
}

static int32_t syscall_mmio_get(int32_t arg0) {
	uint32_t addr = (uint32_t)arg0;
	if(_current_proc->owner > 0 ||
			addr < MMIO_BASE || addr >= (MMIO_BASE+get_mmio_mem_size()))
		return -1;
	return *((int32_t*)(MMIO_BASE+addr));
}

static int32_t syscall_mmio_put(int32_t arg0, int32_t arg1) {
	uint32_t addr = (uint32_t)arg0;
	if(_current_proc->owner > 0 ||
			addr < MMIO_BASE || addr >= (MMIO_BASE+get_mmio_mem_size()))
		return -1;
	*((int32_t*)(MMIO_BASE+addr)) = arg1;
	return 0;
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

static int32_t syscall_fb_info(int arg0) {
	if(_current_proc->owner >= 0)
		return -1;

	fb_info_t* info = fb_get_info();
	if(info == NULL)
		return -1;
	memcpy((fb_info_t*)arg0, info, sizeof(fb_info_t));
	return 0;
}

static int32_t syscall_fb_write(int arg0, int arg1) {
	if(_current_proc->owner >= 0)
		return -1;

	fb_info_t* info = fb_get_info();
	if(info == NULL)
		return -1;

	void* buf = (void*)arg0;
	int32_t sz = (info->depth/8) * info->width * info->height;
	if(arg1 > sz)
		arg1 = sz;
	memcpy((void*)info->pointer, buf, (uint32_t)arg1);
	return arg1;
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

static int32_t syscall_thread(int32_t arg0, int32_t arg1) {
	process_t* proc = kfork(TYPE_THREAD);
	if(proc == NULL)
		return -1;
	proc->context[RESTART_ADDR] = (uint32_t)arg0;
	proc->context[R0] = (uint32_t)arg1;
	return proc->pid;
}

static int32_t syscall_wait(int32_t arg0) {
	process_t *proc = proc_get(arg0);
	if (proc->state != UNUSED) {
		_current_proc->wait_pid = arg0;
		_current_proc->state = SLEEPING;
		schedule();
	}
	return 0;
}

static int32_t syscall_yield() {
	schedule();
	return 0;
}

static int32_t syscall_pmalloc(int32_t arg0) {
	char* p = (char*)pmalloc((uint32_t)arg0);
	return (int)p;
}

static int32_t syscall_pfree(int32_t arg0) {
	char* p = (char*)arg0;
	trunk_free(&_current_proc->space->malloc_man, p);
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

static int32_t syscall_readfile_initrd(int32_t arg0, int32_t arg1, int32_t arg2) {
	if(_current_proc->owner >= 0)
		return 0;

	const char* name = (const char*)arg0;
	int32_t fsize = 0;
	int32_t rdSize = *(int*)arg2;
	*(int*)arg2 = 0;

	const char*p = read_initrd(name, &fsize);
	if(p == NULL || fsize == 0)
		return 0;

	int32_t restSize = fsize - arg1; /*arg1: seek*/
	if(restSize <= 0) {
		return 0;
	}

	if(rdSize <= 0 || rdSize > restSize)
		rdSize = restSize;


	char* ret = (char*)pmalloc(rdSize);
	if(ret == NULL)
		return 0;
	memcpy(ret, p+arg1, rdSize);
	*((int*)arg2) = rdSize;
	return (int)ret;
}

static int32_t syscall_clone_initrd() {
	return (int32_t)clone_initrd();
}

static int32_t syscall_kdb(int32_t arg0) {
	return arg0;
}

static int32_t syscall_pf_open(int32_t arg0, int32_t arg1) {
	process_t* proc = _current_proc;
	if(proc == NULL)
		return -1;

	k_file_t* kf = kf_open((uint32_t)arg0);
	if(kf == NULL)
		return -1;

	kf_ref(kf, arg1);

	int32_t i;
	for(i=0; i<FILE_MAX; i++) {
		if(proc->space->files[i].kf == NULL) {
			proc->space->files[i].kf = kf;
			proc->space->files[i].flags = arg1;
			proc->space->files[i].seek = 0;
			return i;
		}
	}

	kf_unref(kf, arg1);
	return -1;
}

static int32_t syscall_pf_close(int32_t arg0) {
	process_t* proc = _current_proc;
	if(proc == NULL)
		return -1;

	int32_t fd = arg0;
	if(fd < 0 || fd >= FILE_MAX)
		return -1;

	kf_unref(proc->space->files[fd].kf, proc->space->files[fd].flags);

	proc->space->files[fd].kf = NULL;
	proc->space->files[fd].flags = 0;
	proc->space->files[fd].seek = 0;
	return  0;
}

static int32_t syscall_pf_seek(int32_t arg0, int32_t arg1) {
	process_t* proc = _current_proc;
	if(proc == NULL)
		return -1;

	int32_t fd = arg0;
	if(fd < 0 || fd >= FILE_MAX)
		return -1;

	k_file_t* kf = proc->space->files[fd].kf;
	if(kf == NULL || kf->node_addr == 0)
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

	k_file_t* kf = proc->space->files[fd].kf;
	if(kf == NULL || kf->node_addr == 0)
		return -1;
	return proc->space->files[fd].seek;
}

static int32_t syscall_pf_node_addr(int32_t arg0, int32_t arg1) {
	process_t* proc = proc_get(arg0);
	if(proc == NULL || arg1 < 0 || arg1>= FILE_MAX)
		return -1;

	k_file_t* kf = proc->space->files[arg1].kf;
	if(kf == NULL)
		return -1;

	return (int32_t)kf->node_addr;
}

static int32_t syscall_kserv_reg(int32_t arg0) {
	return kserv_reg((const char*)arg0);
}

static int32_t syscall_kserv_get(int32_t arg0) {
	return kserv_get((const char*)arg0);
}

static int32_t syscall_get_cwd(int32_t arg0, int32_t arg1) {
	char* pwd = (char*)arg0;
	strncpy(pwd, _current_proc->pwd,
		arg1 < NAME_MAX ? arg1: NAME_MAX);
	return (int)pwd;
}

static int32_t syscall_set_cwd(int32_t arg0) {
	const char* pwd = (const char*)arg0;
	strncpy(_current_proc->pwd, pwd, NAME_MAX);
	return 0;
}

static int32_t syscall_get_cmd(int32_t arg0, int32_t arg1) {
	char* cmd = (char*)arg0;
	strncpy(cmd, _current_proc->cmd, arg1);
	return arg0;
}

static int32_t syscall_set_uid(int32_t arg0, int32_t arg1) {
	if(arg1 < 0 || _current_proc->owner >= 0) {/*current process not kernel proc*/
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

static int32_t syscall_system_cmd(int32_t arg0, int32_t arg1) { 
	//arg0: command id; arg1: command arg
	return system_cmd(arg0, arg1);
}

static int32_t (*const _syscallHandler[])() = {
	[SYSCALL_KDB] = syscall_kdb,
	[SYSCALL_UART_PUTCH] = syscall_uart_putch,
	[SYSCALL_UART_GETCH] = syscall_uart_getch,

	[SYSCALL_MMIO_GET] = syscall_mmio_get,
	[SYSCALL_MMIO_PUT] = syscall_mmio_put,

	[SYSCALL_SHM_ALLOC] = syscall_shm_alloc,
	[SYSCALL_SHM_FREE] = syscall_shm_free,
	[SYSCALL_SHM_MAP] = syscall_shm_map,
	[SYSCALL_SHM_UNMAP] = syscall_shm_unmap,

	[SYSCALL_FB_INFO] = syscall_fb_info,
	[SYSCALL_FB_WRITE] = syscall_fb_write,

	[SYSCALL_FORK] = syscall_fork,
	[SYSCALL_GETPID] = syscall_getpid,
	[SYSCALL_EXEC_ELF] = syscall_exec_elf,
	[SYSCALL_WAIT] = syscall_wait,
	[SYSCALL_YIELD] = syscall_yield,
	[SYSCALL_EXIT] = syscall_exit,

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

	[SYSCALL_INITRD_READ_FILE] = syscall_readfile_initrd,
	[SYSCALL_INITRD_CLONE] = syscall_clone_initrd,

	[SYSCALL_PFILE_GET_SEEK] = syscall_pf_get_seek,
	[SYSCALL_PFILE_SEEK] = syscall_pf_seek,
	[SYSCALL_PFILE_OPEN] = syscall_pf_open,
	[SYSCALL_PFILE_CLOSE] = syscall_pf_close,
	[SYSCALL_PFILE_NODE] = syscall_pf_node_addr,

	[SYSCALL_KSERV_REG] = syscall_kserv_reg,
	[SYSCALL_KSERV_GET] = syscall_kserv_get,

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

