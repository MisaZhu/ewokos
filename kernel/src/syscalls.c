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
#include <dev/initrd.h>
#include <dev/fb.h>

static int32_t syscall_uartPutch(int32_t c) {
	uart_putch(c);
	return 0;
}

static int32_t syscall_uartGetch() {
	int32_t r = uart_getch();
	return r;
}

static int32_t syscall_mmioGet(int32_t arg0) {
	uint32_t addr = (uint32_t)arg0;
	if(_current_proc->owner > 0 ||
			addr < MMIO_BASE || addr >= (MMIO_BASE+get_mmio_mem_size()))
		return -1;
	return *((int32_t*)(MMIO_BASE+addr));
}

static int32_t syscall_mmioPut(int32_t arg0, int32_t arg1) {
	uint32_t addr = (uint32_t)arg0;
	if(_current_proc->owner > 0 ||
			addr < MMIO_BASE || addr >= (MMIO_BASE+get_mmio_mem_size()))
		return -1;
	*((int32_t*)(MMIO_BASE+addr)) = arg1;
	return 0;
}

static int32_t syscall_shmAlloc(int arg0) {
	return shm_alloc((uint32_t)arg0);
}

static int32_t syscall_shmMap(int arg0) {
	return (int32_t)shm_proc_map(_current_proc->pid, arg0);
}

static int32_t syscall_shmFree(int arg0) {
	shm_free(arg0);
	return 0;
}

static int32_t syscall_shmUnmap(int arg0) {
	return shm_proc_unmap(_current_proc->pid, arg0);
}

static int32_t syscall_fbInfo(int arg0) {
	if(_current_proc->owner >= 0)
		return -1;

	fb_info_t* info = _fb_get_info();
	if(info == NULL)
		return -1;
	memcpy((fb_info_t*)arg0, info, sizeof(fb_info_t));
	return 0;
}

static int32_t syscall_fbWrite(int arg0, int arg1) {
	if(_current_proc->owner >= 0)
		return -1;

	fb_info_t* info = _fb_get_info();
	if(info == NULL)
		return -1;

	void* buf = (void*)arg0;
	int32_t sz = (info->depth/8) * info->width * info->height;
	if(arg1 > sz)
		arg1 = sz;
	memcpy((void*)info->pointer, buf, (uint32_t)arg1);
	return arg1;
}

static int32_t syscall_execElf(int32_t arg0, int32_t arg1, int32_t arg2) {
	const char*cmd = (const char*)arg0;
	const char*p = (const char*)arg1;

	if(p == NULL || cmd == NULL)
		return -1;
		
	strncpy(_current_proc->cmd, cmd, CMD_MAX);

	if(!proc_load(_current_proc, p, (uint32_t)arg2))
		return -1;

	proc_start(_current_proc);
	return 0;
}

static int32_t syscall_fork(void) {
	return kfork();
}

static int32_t syscall_getpid(void) {
	return _current_proc->pid;
}

static int32_t syscall_exit(int32_t arg0) {
	(void)arg0;
	proc_exit();
	return 0;
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
	trunk_free(&_current_proc->malloc_man, p);
	return 0;
}

static int32_t syscall_ipcOpen(int32_t arg0, int32_t arg1) {
	return ipc_open(arg0, arg1);
}

static int32_t syscall_ipcReady() {
	return ipc_ready();
}

static int32_t syscall_ipcClose(int32_t arg0) {
	return ipc_close(arg0);
}

static int32_t syscall_ipcWrite(int32_t arg0, int32_t arg1, int32_t arg2) {
	return ipc_write(arg0, (void*)arg1, arg2);
}

static int32_t syscall_ipcRead(int32_t arg0, int32_t arg1, int32_t arg2) {
	return ipc_read(arg0, (void*)arg1, arg2);
}

static int32_t syscall_ipcRing(int32_t arg0) {
	return ipc_ring(arg0);
}

static int32_t syscall_ipcPeer(int32_t arg0) {
	return ipc_peer(arg0);
}

static int32_t syscall_readFileInitRD(int32_t arg0, int32_t arg1, int32_t arg2) {
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

static int32_t syscall_cloneInitRD() {
	return (int32_t)clone_initrd();
}

static int32_t syscall_kdb(int32_t arg0) {
	return arg0;
}

static int32_t syscall_pfOpen(int32_t arg0, int32_t arg1) {
	process_t* proc = _current_proc;
	if(proc == NULL)
		return -1;

	k_file_t* kf = kf_open((uint32_t)arg0);
	if(kf == NULL)
		return -1;

	kf_ref(kf, arg1);

	int32_t i;
	for(i=0; i<FILE_MAX; i++) {
		if(proc->files[i].kf == NULL) {
			proc->files[i].kf = kf;
			proc->files[i].flags = arg1;
			proc->files[i].seek = 0;
			return i;
		}
	}

	kf_unref(kf, arg1);
	return -1;
}

static int32_t syscall_pfClose(int32_t arg0) {
	process_t* proc = _current_proc;
	if(proc == NULL)
		return -1;

	int32_t fd = arg0;
	if(fd < 0 || fd >= FILE_MAX)
		return -1;

	kf_unref(proc->files[fd].kf, proc->files[fd].flags);

	proc->files[fd].kf = NULL;
	proc->files[fd].flags = 0;
	proc->files[fd].seek = 0;
	return  0;
}

static int32_t syscall_pfSeek(int32_t arg0, int32_t arg1) {
	process_t* proc = _current_proc;
	if(proc == NULL)
		return -1;

	int32_t fd = arg0;
	if(fd < 0 || fd >= FILE_MAX)
		return -1;

	k_file_t* kf = proc->files[fd].kf;
	if(kf == NULL || kf->node_addr == 0)
		return -1;
	proc->files[fd].seek = arg1;
	return arg1;
}

static int32_t syscall_pfGetSeek(int32_t arg0) {
	process_t* proc = _current_proc;
	if(proc == NULL)
		return -1;

	int32_t fd = arg0;
	if(fd < 0 || fd >= FILE_MAX)
		return -1;

	k_file_t* kf = proc->files[fd].kf;
	if(kf == NULL || kf->node_addr == 0)
		return -1;
	return proc->files[fd].seek;
}

static int32_t syscall_pfNodeAddr(int32_t arg0, int32_t arg1) {
	process_t* proc = proc_get(arg0);
	if(proc == NULL || arg1 < 0 || arg1>= FILE_MAX)
		return -1;

	k_file_t* kf = proc->files[arg1].kf;
	if(kf == NULL)
		return -1;

	return (int32_t)kf->node_addr;
}

static int32_t syscall_kservReg(int32_t arg0) {
	return kserv_reg((const char*)arg0);
}

static int32_t syscall_kservGet(int32_t arg0) {
	return kserv_get((const char*)arg0);
}

static int32_t getProcs(bool owner) {
	int32_t res = 0;
	for(int32_t i=0; i<PROCESS_COUNT_MAX; i++) {
		if(_process_table[i].state != UNUSED && 
				(_current_proc->owner == 0 || 
				owner != true || 
				_process_table[i].owner == _current_proc->owner)) {
			res++;
		}
	}
	return res;
}

static int32_t syscall_getProcs(int32_t arg0, int32_t arg1) {
	bool owner = (bool)arg1;
	int32_t num = getProcs(owner);
	if(num == 0)
		return 0;

	/*need to be freed later used!*/
	proc_info_t* procs = (proc_info_t*)pmalloc(sizeof(proc_info_t)*num);
	if(procs == NULL)
		return 0;

	int32_t j = 0;
	for(int32_t i=0; i<PROCESS_COUNT_MAX && j<num; i++) {
		if(_process_table[i].state != UNUSED && 
				(_current_proc->owner == 0 ||
				owner != true ||
				 _process_table[i].owner == _current_proc->owner)) {
			procs[j].pid = _process_table[i].pid;	
			procs[j].father_pid = _process_table[i].father_pid;	
			procs[j].owner = _process_table[i].owner;	
			procs[j].heap_size = _process_table[i].heap_size;	
			strcpy(procs[j].cmd, _process_table[i].cmd);
			j++;
		}
	}

	*(int*)arg0 = j;
	return (int)procs;
}

static int32_t syscall_getCWD(int32_t arg0, int32_t arg1) {
	char* pwd = (char*)arg0;
	strncpy(pwd, _current_proc->pwd,
		arg1 < NAME_MAX ? arg1: NAME_MAX);
	return (int)pwd;
}

static int32_t syscall_setCWD(int32_t arg0) {
	const char* pwd = (const char*)arg0;
	strncpy(_current_proc->pwd, pwd, NAME_MAX);
	return 0;
}

static int32_t syscall_getCmd(int32_t arg0, int32_t arg1) {
	char* cmd = (char*)arg0;
	strncpy(cmd, _current_proc->cmd, arg1);
	return arg0;
}

static int32_t syscall_setUID(int32_t arg0, int32_t arg1) {
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

static int32_t syscall_getUID(int32_t arg0) {
	process_t* proc = proc_get(arg0);
	if(proc == NULL)
		return -1;
	return proc->owner;
}

static int32_t (*const _syscallHandler[])() = {
	[SYSCALL_KDB] = syscall_kdb,
	[SYSCALL_UART_PUTCH] = syscall_uartPutch,
	[SYSCALL_UART_GETCH] = syscall_uartGetch,

	[SYSCALL_MMIO_GET] = syscall_mmioGet,
	[SYSCALL_MMIO_PUT] = syscall_mmioPut,

	[SYSCALL_SHM_ALLOC] = syscall_shmAlloc,
	[SYSCALL_SHM_FREE] = syscall_shmFree,
	[SYSCALL_SHM_MAP] = syscall_shmMap,
	[SYSCALL_SHM_UNMAP] = syscall_shmUnmap,

	[SYSCALL_FB_INFO] = syscall_fbInfo,
	[SYSCALL_FB_WRITE] = syscall_fbWrite,

	[SYSCALL_FORK] = syscall_fork,
	[SYSCALL_GETPID] = syscall_getpid,
	[SYSCALL_EXEC_ELF] = syscall_execElf,
	[SYSCALL_WAIT] = syscall_wait,
	[SYSCALL_YIELD] = syscall_yield,
	[SYSCALL_EXIT] = syscall_exit,

	[SYSCALL_PMALLOC] = syscall_pmalloc,
	[SYSCALL_PFREE] = syscall_pfree,

	[SYSCALL_GET_CMD] = syscall_getCmd,

	[SYSCALL_IPC_OPEN] = syscall_ipcOpen,
	[SYSCALL_IPC_CLOSE] = syscall_ipcClose,
	[SYSCALL_IPC_WRITE] = syscall_ipcWrite,
	[SYSCALL_IPC_READY] = syscall_ipcReady,
	[SYSCALL_IPC_READ] = syscall_ipcRead,
	[SYSCALL_IPC_RING] = syscall_ipcRing,
	[SYSCALL_IPC_PEER] = syscall_ipcPeer,

	[SYSCALL_INITRD_READ_FILE] = syscall_readFileInitRD,
	[SYSCALL_INITRD_CLONE] = syscall_cloneInitRD,

	[SYSCALL_PFILE_GET_SEEK] = syscall_pfGetSeek,
	[SYSCALL_PFILE_SEEK] = syscall_pfSeek,
	[SYSCALL_PFILE_OPEN] = syscall_pfOpen,
	[SYSCALL_PFILE_CLOSE] = syscall_pfClose,
	[SYSCALL_PFILE_NODE] = syscall_pfNodeAddr,

	[SYSCALL_KSERV_REG] = syscall_kservReg,
	[SYSCALL_KSERV_GET] = syscall_kservGet,

	[SYSCALL_GET_PROCS] = syscall_getProcs,
	[SYSCALL_GET_CWD] = syscall_getCWD,
	[SYSCALL_SET_CWD] = syscall_setCWD,

	[SYSCALL_SET_UID] = syscall_setUID,
	[SYSCALL_GET_UID] = syscall_getUID,
};

/* kernel side of system calls. */
int32_t handle_syscall(int32_t code, int32_t arg0, int32_t arg1, int32_t arg2) {
	return _syscallHandler[code](arg0, arg1, arg2);
}

