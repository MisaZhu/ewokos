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
#include <sramdisk.h>
#include <dev/fb.h>

static int32_t syscall_uartPutch(int32_t c) {
	uartPutch(c);
	return 0;
}

static int32_t syscall_uartGetch() {
	int32_t r = uartGetch();
	return r;
}

static int32_t syscall_mmioGet(int32_t arg0) {
	uint32_t addr = (uint32_t)arg0;
	if(_currentProcess->owner > 0 ||
			addr < MMIO_BASE || addr >= (MMIO_BASE+getMMIOMemSize()))
		return -1;
	return *((int32_t*)(MMIO_BASE+addr));
}

static int32_t syscall_mmioPut(int32_t arg0, int32_t arg1) {
	uint32_t addr = (uint32_t)arg0;
	if(_currentProcess->owner > 0 ||
			addr < MMIO_BASE || addr >= (MMIO_BASE+getMMIOMemSize()))
		return -1;
	*((int32_t*)(MMIO_BASE+addr)) = arg1;
	return 0;
}

static int32_t syscall_shmAlloc(int arg0) {
	return shmalloc((uint32_t)arg0);
}

static int32_t syscall_shmMap(int arg0) {
	return (int32_t)shmProcMap(_currentProcess->pid, arg0);
}

static int32_t syscall_shmFree(int arg0) {
	shmfree(arg0);
	return 0;
}

static int32_t syscall_shmUnmap(int arg0) {
	return shmProcUnmap(_currentProcess->pid, arg0);
}

static int32_t syscall_fbInfo(int arg0) {
	if(_currentProcess->owner >= 0)
		return -1;

	FBInfoT* info = fbGetInfo();
	if(info == NULL)
		return -1;
	memcpy((FBInfoT*)arg0, info, sizeof(FBInfoT));
	return 0;
}

static int32_t syscall_fbWrite(int arg0, int arg1) {
	if(_currentProcess->owner >= 0)
		return -1;

	FBInfoT* info = fbGetInfo();
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
		
	strncpy(_currentProcess->cmd, cmd, CMD_MAX);

	if(!procLoad(_currentProcess, p, (uint32_t)arg2))
		return -1;

	procStart(_currentProcess);
	return 0;
}

static int32_t syscall_fork(void) {
	return kfork();
}

static int32_t syscall_getpid(void) {
	return _currentProcess->pid;
}

static int32_t syscall_exit(int32_t arg0) {
	(void)arg0;
	procExit();
	return 0;
}

static int32_t syscall_wait(int32_t arg0) {
	ProcessT *proc = procGet(arg0);
	if (proc->state != UNUSED) {
		_currentProcess->waitPid = arg0;
		_currentProcess->state = SLEEPING;
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
	trunkFree(&_currentProcess->mallocMan, p);
	return 0;
}

static int32_t syscall_ipcOpen(int32_t arg0, int32_t arg1) {
	return ipcOpen(arg0, arg1);
}

static int32_t syscall_ipcReady() {
	return ipcReady();
}

static int32_t syscall_ipcClose(int32_t arg0) {
	return ipcClose(arg0);
}

static int32_t syscall_ipcWrite(int32_t arg0, int32_t arg1, int32_t arg2) {
	return ipcWrite(arg0, (void*)arg1, arg2);
}

static int32_t syscall_ipcRead(int32_t arg0, int32_t arg1, int32_t arg2) {
	return ipcRead(arg0, (void*)arg1, arg2);
}

static int32_t syscall_ipcRing(int32_t arg0) {
	return ipcRing(arg0);
}

static int32_t syscall_ipcPeer(int32_t arg0) {
	return ipcPeer(arg0);
}

static int32_t syscall_readFileInitRD(int32_t arg0, int32_t arg1, int32_t arg2) {
	if(_currentProcess->owner >= 0)
		return 0;

	const char* name = (const char*)arg0;
	int32_t fsize = 0;
	int32_t rdSize = *(int*)arg2;
	*(int*)arg2 = 0;

	const char*p = ramdiskRead(&_initRamDisk, name, &fsize);
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
	void * ret = (void*)pmalloc(_initRamDiskSize);
	memcpy(ret, _initRamDiskBase, _initRamDiskSize);

	ramdiskClose(&_initRamDisk, kmfree);
	kmfree(_initRamDiskBase);
	_initRamDiskBase = NULL;
	_initRamDiskSize = 0;

	return (int32_t)ret;
}

static int32_t syscall_kdb(int32_t arg0) {
	return arg0;
}

static int32_t syscall_pfOpen(int32_t arg0, int32_t arg1) {
	ProcessT* proc = _currentProcess;
	if(proc == NULL)
		return -1;

	KFileT* kf = kfOpen((uint32_t)arg0);
	if(kf == NULL)
		return -1;

	kfRef(kf, arg1);

	int32_t i;
	for(i=0; i<FILE_MAX; i++) {
		if(proc->files[i].kf == NULL) {
			proc->files[i].kf = kf;
			proc->files[i].flags = arg1;
			proc->files[i].seek = 0;
			return i;
		}
	}

	kfUnref(kf, arg1);
	return -1;
}

static int32_t syscall_pfClose(int32_t arg0) {
	ProcessT* proc = _currentProcess;
	if(proc == NULL)
		return -1;

	int32_t fd = arg0;
	if(fd < 0 || fd >= FILE_MAX)
		return -1;

	kfUnref(proc->files[fd].kf, proc->files[fd].flags);

	proc->files[fd].kf = NULL;
	proc->files[fd].flags = 0;
	proc->files[fd].seek = 0;
	return  0;
}

static int32_t syscall_pfSeek(int32_t arg0, int32_t arg1) {
	ProcessT* proc = _currentProcess;
	if(proc == NULL)
		return -1;

	int32_t fd = arg0;
	if(fd < 0 || fd >= FILE_MAX)
		return -1;

	KFileT* kf = proc->files[fd].kf;
	if(kf == NULL || kf->nodeAddr == 0)
		return -1;
	proc->files[fd].seek = arg1;
	return arg1;
}

static int32_t syscall_pfGetSeek(int32_t arg0) {
	ProcessT* proc = _currentProcess;
	if(proc == NULL)
		return -1;

	int32_t fd = arg0;
	if(fd < 0 || fd >= FILE_MAX)
		return -1;

	KFileT* kf = proc->files[fd].kf;
	if(kf == NULL || kf->nodeAddr == 0)
		return -1;
	return proc->files[fd].seek;
}

static int32_t syscall_pfNodeAddr(int32_t arg0, int32_t arg1) {
	ProcessT* proc = procGet(arg0);
	if(proc == NULL || arg1 < 0 || arg1>= FILE_MAX)
		return -1;

	KFileT* kf = proc->files[arg1].kf;
	if(kf == NULL)
		return -1;

	return (int32_t)kf->nodeAddr;
}

static int32_t syscall_kservReg(int32_t arg0) {
	return kservReg((const char*)arg0);
}

static int32_t syscall_kservGet(int32_t arg0) {
	return kservGet((const char*)arg0);
}

static int32_t getProcs(bool owner) {
	int32_t res = 0;
	for(int32_t i=0; i<PROCESS_COUNT_MAX; i++) {
		if(_processTable[i].state != UNUSED && 
				(_currentProcess->owner == 0 || 
				owner != true || 
				_processTable[i].owner == _currentProcess->owner)) {
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
	ProcInfoT* procs = (ProcInfoT*)pmalloc(sizeof(ProcInfoT)*num);
	if(procs == NULL)
		return 0;

	int32_t j = 0;
	for(int32_t i=0; i<PROCESS_COUNT_MAX && j<num; i++) {
		if(_processTable[i].state != UNUSED && 
				(_currentProcess->owner == 0 ||
				owner != true ||
				 _processTable[i].owner == _currentProcess->owner)) {
			procs[j].pid = _processTable[i].pid;	
			procs[j].fatherPid = _processTable[i].fatherPid;	
			procs[j].owner = _processTable[i].owner;	
			procs[j].heapSize = _processTable[i].heapSize;	
			strcpy(procs[j].cmd, _processTable[i].cmd);
			j++;
		}
	}

	*(int*)arg0 = j;
	return (int)procs;
}

static int32_t syscall_getCWD(int32_t arg0, int32_t arg1) {
	char* pwd = (char*)arg0;
	strncpy(pwd, _currentProcess->pwd,
		arg1 < NAME_MAX ? arg1: NAME_MAX);
	return (int)pwd;
}

static int32_t syscall_setCWD(int32_t arg0) {
	const char* pwd = (const char*)arg0;
	strncpy(_currentProcess->pwd, pwd, NAME_MAX);
	return 0;
}

static int32_t syscall_getCmd(int32_t arg0, int32_t arg1) {
	char* cmd = (char*)arg0;
	strncpy(cmd, _currentProcess->cmd, arg1);
	return arg0;
}

static int32_t syscall_setUID(int32_t arg0, int32_t arg1) {
	if(arg1 < 0 || _currentProcess->owner >= 0) {/*current process not kernel proc*/
		return -1;
	}
	
	ProcessT* proc = procGet(arg0);
	if(proc == NULL) {
		return -1;
	}

	proc->owner = arg1;
	return 0;
}

static int32_t syscall_getUID(int32_t arg0) {
	ProcessT* proc = procGet(arg0);
	if(proc == NULL)
		return -1;
	return proc->owner;
}

/*
static int32_t syscall_vfsAdd(int32_t arg0, int32_t arg1, int32_t arg2) {
	TreeNodeT* nodeTo = (TreeNodeT*)arg0;
	const char* name = (const char*)arg1;
	return (int32_t)vfsAdd(nodeTo, name, (uint32_t)arg2);
}

static int32_t syscall_vfsDel(int32_t arg0) {
	TreeNodeT* node = (TreeNodeT*)arg0;
	return vfsDel(node);
}

static int32_t syscall_vfsInfo(int32_t arg0, int32_t arg1) {
	TreeNodeT* node = (TreeNodeT*)arg0;
	FSInfoT* info = (FSInfoT*)arg1;
	return vfsNodeInfo(node, info);
}

static int32_t syscall_vfsKids(int32_t arg0, int32_t arg1) {
	TreeNodeT* node = (TreeNodeT*)arg0;
	int32_t* num = (int32_t*)arg1;
	return (int32_t)vfsNodeKids(node, num);
}

static int32_t syscall_vfsNodeByName(int32_t arg0) {
	return (int32_t)getNodeByName((const char*)arg0);	
}

static int32_t syscall_vfsMount(int32_t arg0, int32_t arg1, int32_t arg2) {
	const char* fname = (const char*)arg0;
	const char* devName = (const char*)arg1;
	return (int32_t)vfsMount(fname, devName, arg2);
}

static int32_t syscall_vfsMountFile(int32_t arg0, int32_t arg1, int32_t arg2) {
	const char* fname = (const char*)arg0;
	const char* devName = (const char*)arg1;
	TreeNodeT* node = vfsMount(fname, devName, arg2);
	FSN(node)->type = FS_TYPE_FILE;
	return (int32_t)node;
}

static int32_t syscall_vfsUnmount(int32_t arg0) {
	TreeNodeT* node = (TreeNodeT*)arg0;
	return vfsUnmount(node);
}
*/

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
int32_t handleSyscall(int32_t code, int32_t arg0, int32_t arg1, int32_t arg2) {
	return _syscallHandler[code](arg0, arg1, arg2);
}

