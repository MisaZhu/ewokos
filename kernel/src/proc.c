#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <proc.h>
#include <kernel.h>
#include <kstring.h>
#include <system.h>
#include <types.h>
#include <dev/uart.h>
#include <elf.h>
#include <kernel.h>
#include <scheduler.h>

ProcessT _processTable[PROCESS_COUNT_MAX];

__attribute__((__aligned__(PAGE_DIR_SIZE)))
static PageDirEntryT _processVM[PROCESS_COUNT_MAX][PAGE_DIR_NUM];

ProcessT* _currentProcess = NULL;

/* proc_init initializes the process sub-system. */
void procInit(void)
{
	for (int i = 0; i < PROCESS_COUNT_MAX; i++)
		_processTable[i].state = UNUSED;
	_currentProcess = NULL;
}

/* proc_exapnad_memory expands the heap size of the given process. */
bool procExpandMemory(void *p, int pageCount)
{
	ProcessT* proc = (ProcessT*)p;	
	for (int i = 0; i < pageCount; i++) {
		char *page = kalloc();
		if(page == NULL) {
			procShrinkMemory(proc, i);
			return false;
		}
		memset(page, 0, PAGE_SIZE);

		mapPage(proc->vm, 
				proc->heapSize,
				V2P(page),
				AP_RW_RW);
		proc->heapSize += PAGE_SIZE;
	}
	return true;
}

/* proc_shrink_memory shrinks the heap size of the given process. */
void procShrinkMemory(void* p, int pageCount)
{
	ProcessT* proc = (ProcessT*)p;	
	for (int i = 0; i < pageCount; i++) {
		uint32_t virtualAddr = proc->heapSize - PAGE_SIZE;
		uint32_t physicalAddr = resolvePhyAddress(proc->vm, virtualAddr);

		//get the kernel address for kalloc/kfree
		uint32_t kernelAddr = P2V(physicalAddr);
		kfree((void *) kernelAddr);

		unmapPage(proc->vm, virtualAddr);

		proc->heapSize -= PAGE_SIZE;
		if (proc->heapSize == 0) {
			break;
		}
	}
}

static void* procGetMemTail(void* p) {
	ProcessT* proc = (ProcessT*)p;	
	return (void*)proc->heapSize;
}

/* proc_creates allocates a new process and returns it. */
ProcessT *procCreate(void)
{
	ProcessT *proc = NULL;
	int index = -1;
	PageDirEntryT *vm = NULL;
	//char *kernelStack = NULL;
	char *userStack = NULL;

	for (int i = 0; i < PROCESS_COUNT_MAX; i++) {
		if (_processTable[i].state == UNUSED) {
			index = i;
			break;
		}
	}

	if (index < 0)
		return NULL;
	proc = &_processTable[index];

	//kernelStack = kalloc();
	userStack = kalloc();

	vm = _processVM[index];
	setKernelVM(vm);

	/*mapPage(vm, 
		KERNEL_STACK_BOTTOM,
		V2P(kernelStack),
		AP_RW_R);
		*/

	mapPage(vm,
		USER_STACK_BOTTOM,
		V2P(userStack),
		AP_RW_RW);

	proc->pid = index;
	proc->fatherPid = 0;
	proc->owner = -1;
	proc->cmd[0] = 0;
	strcpy(proc->pwd, "/");

	proc->state = CREATED;
	proc->vm = vm;
	proc->heapSize = 0;

	//proc->kernelStack = kernelStack;
	proc->userStack = userStack;

	proc->waitPid = -1;
	proc->mallocMan.arg = (void*)proc;
	proc->mallocMan.mHead = 0;
	proc->mallocMan.mTail = 0;
	proc->mallocMan.expand = procExpandMemory;
	proc->mallocMan.shrink = procShrinkMemory;
	proc->mallocMan.getMemTail = procGetMemTail;

	memset(&proc->files, 0, sizeof(ProcFileT)*FILE_MAX);
	return proc;
}

int *getCurrentContext(void)
{
	return _currentProcess->context;
}

/* proc_free frees all resources allocated by proc. */
void procFree(ProcessT *proc)
{
	/*free file info*/
	for(int i=0; i<FILE_MAX; i++) {
		KFileT* kf = proc->files[i].kf;
		if(kf != NULL) {
			kfUnref(kf, proc->files[i].flags); //unref the kernel file table.
		}
	}

	ipcCloseAll();
	//kfree(proc->kernelStack);
	kfree(proc->userStack);
	procShrinkMemory(proc, proc->heapSize / PAGE_SIZE);
	freePageTables(proc->vm);
	proc->state = UNUSED;
}

#define MODE_MASK 0x1f
#define MODE_USER 0x10
#define DIS_INT (1<<7)

uint32_t cpsrUser() {
    uint32_t val;
    __asm__ volatile("mrs %[v], cpsr": [v]"=r" (val)::);
    val &= ~MODE_MASK;
    val |= MODE_USER;
    val &= ~DIS_INT;
    return val;
}

/* proc_load loads the given ELF process image into the given process. */
bool procLoad(ProcessT *proc, const char *procImage)
{
	int progHeaderOffset = 0;
	int progHeaderCount = 0;
	int i = 0;

	struct ElfHeader *header = (struct ElfHeader *) procImage;
	if (header->type != ELFTYPE_EXECUTABLE)
		return false;

	progHeaderOffset = header->phoff;
	progHeaderCount = header->phnum;

	for (i = 0; i < progHeaderCount; i++) {
		uint32_t j = 0;
		struct ElfProgramHeader *header = (void *) (procImage + progHeaderOffset);

		/* make enough room for this section */
		while (proc->heapSize < header->vaddr + header->memsz) {
			if(!procExpandMemory(proc, 1)) {
				uartPuts("Panic: proc expand memory failed!!\n");
				return false;
			}
		}

		/* copy the section */
		for (j = 0; j < header->memsz; j++) {
			int vaddr = header->vaddr + j; /*vaddr in elf*/
			int paddr = resolvePhyAddress(proc->vm, vaddr); /*trans to phyaddr by proc's page dir*/
			char *ptr = (char *) P2V(paddr); /*trans the phyaddr to vaddr now in kernel page dir*/
			/*copy from elf to vaddrKernel(=phyaddr=vaddrProc=vaddrElf)*/
			int imageOff = header->off + j;
			*ptr = procImage[imageOff]; 
		}

		progHeaderOffset += sizeof(struct ElfProgramHeader);
	}

	proc->mallocMan.mHead = 0;
	proc->mallocMan.mTail = 0;
	
	proc->entry = (EntryFunctionT) header->entry;
	proc->state = READY;

	memset(proc->context, 0, sizeof(proc->context));
	proc->context[CPSR] = cpsrUser(); //CPSR 0x10 for user mode
	proc->context[RESTART_ADDR] = (int) proc->entry;
	proc->context[SP] = USER_STACK_BOTTOM + PAGE_SIZE;

	return true;
}

/* proc_start starts running the given process. */
void procStart(ProcessT *proc)
{
	_currentProcess = proc;

	__setTranslationTableBase((uint32_t) V2P(proc->vm));

	/* clear TLB */
	__asm__ volatile("mov R4, #0");
	__asm__ volatile("MCR p15, 0, R4, c8, c7, 0");

	__switchToContext(proc->context);
}

ProcessT* procGet(int pid) 
{
	if(pid < 0 || pid >= PROCESS_COUNT_MAX)
		return NULL;

	return &_processTable[pid];
}

int kfork(void)
{
	ProcessT *child = NULL;
	ProcessT *parent = _currentProcess;
	uint32_t i = 0;

	child = procCreate();
	if(!procExpandMemory(child, parent->heapSize / PAGE_SIZE)) {
		uartPuts("Panic: proc expand memory failed!!\n");
		return -1;
	}

	/* copy parent's memory to child's memory */
	for (i = 0; i < parent->heapSize; i++) {
		int childPAddr = resolvePhyAddress(child->vm, i);
		char *childPtr = (char *) P2V(childPAddr);
		char *parentPtr = (char *) i;

		*childPtr = *parentPtr;
	}

	/* copy parent's stack to child's stack */
	//memcpy(child->kernelStack, parent->kernelStack, PAGE_SIZE);
	memcpy(child->userStack, parent->userStack, PAGE_SIZE);

	/* copy parent's context to child's context */
	memcpy(child->context, parent->context, sizeof(child->context));

	/*pmalloc list*/
	child->mallocMan = parent->mallocMan;
	child->mallocMan.arg = child;

	/* set return value of fork in child to 0 */
	child->context[R0] = 0;

	/* child is ready to run */
	child->state = READY;
	
	/*set father*/
	child->fatherPid = parent->pid;

	/*same owner*/
	child->owner = parent->owner;

	/*cmd*/
	strcpy(child->cmd, parent->cmd);

	/*pwd*/
	strcpy(child->pwd, parent->pwd);

	/*file info*/
	for(i=0; i<FILE_MAX; i++) {
		KFileT* kf = parent->files[i].kf;
		if(kf != NULL) {
			child->files[i].kf = kf;
			child->files[i].flags = parent->files[i].flags;
			child->files[i].seek = parent->files[i].seek;
			kfRef(kf, child->files[i].flags); //ref the kernel file table.
		}
	}
	/* return pid of child to the parent. */
	return child->pid;
}

void procExit() {
	int i;

	if (_currentProcess == NULL)
		return;

	__setTranslationTableBase((uint32_t) V2P(_kernelVM));
	__asm__ volatile("ldr sp, = _initStack");
	__asm__ volatile("add sp, #4096");

	for (i = 0; i < PROCESS_COUNT_MAX; i++) {
		ProcessT *proc = &_processTable[i];

		if (proc->state == SLEEPING &&
				proc->waitPid == _currentProcess->pid) {
			proc->waitPid = -1;
			proc->state = READY;
		}
	}

	procFree(_currentProcess);
	_currentProcess = NULL;

	schedule();
	return;
}

void _abortEntry() {
	procExit();
}

void* pmalloc(uint32_t size) {
	if(_currentProcess == NULL || size == 0)
		return NULL;
	return trunkMalloc(&_currentProcess->mallocMan, size);
}
