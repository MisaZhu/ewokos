#include <mmu.h>
#include <proc.h>
#include <kernel.h>
#include <kalloc.h>
#include <lib/string.h>
#include <system.h>
#include <types.h>
#include <elf.h>

ProcessT _processTable[PROCESS_COUNT_MAX];

__attribute__((__aligned__(PAGE_DIR_SIZE)))
PageDirEntryT _processVM[PROCESS_COUNT_MAX][PAGE_DIR_NUM];

ProcessT* _currentProcess = NULL;

/* proc_init initializes the process sub-system. */
void procInit(void)
{
	for (int i = 0; i < PROCESS_COUNT_MAX; i++)
		_processTable[i].state = UNUSED;
	_currentProcess = NULL;
}

/* proc_creates allocates a new process and returns it. */
ProcessT *procCreate(void)
{
	ProcessT *proc = NULL;
	int pid = -1;
	PageDirEntryT *vm = NULL;
	char *kernelStack = NULL;
	char *userStack = NULL;

	for (int i = 0; i < PROCESS_COUNT_MAX; i++) {
		if (_processTable[i].state == UNUSED) {
			pid = i + 1;
			break;
		}
	}

	if (pid == -1)
		return NULL;

	kernelStack = kalloc();
	userStack = kalloc();

	vm = _processVM[pid - 1];
	setKernelVM(vm);

	mapPages(vm, (MemoryMapT){
		KERNEL_STACK_BOTTOM,
		V2P(kernelStack),
		V2P(kernelStack) + PAGE_SIZE,
		AP_RW_R
	});

	mapPages(vm, (MemoryMapT){
		USER_STACK_BOTTOM,
		V2P(userStack),
		V2P(userStack) + PAGE_SIZE,
		AP_RW_RW
	});


	proc = &_processTable[pid - 1];
	proc->pid = pid;
	proc->state = CREATED;
	proc->vm = vm;
	proc->heapSize = 0;
	proc->kernelStack = kernelStack;
	proc->userStack = userStack;
	proc->waitPid = -1;

	return proc;
}

int *getCurrentContext(void)
{
	return _currentProcess->context;
}

/* proc_exapnad_memory expands the heap size of the given process. */
void procExpandMemory(ProcessT *proc, int pageCount)
{
	for (int i = 0; i < pageCount; i++) {
		char *page = kalloc();
		memset(page, 0, PAGE_SIZE);

		mapPages(proc->vm, (MemoryMapT){
				proc->heapSize,
				V2P(page),
				V2P(page) + PAGE_SIZE,
				AP_RW_RW
				});
		proc->heapSize += PAGE_SIZE;
	}
}

/* proc_shrink_memory shrinks the heap size of the given process. */
void procShrinkMemory(ProcessT *proc, int pageCount)
{
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

/* proc_free frees all resources allocated by proc. */
void procFree(ProcessT *proc)
{
	kfree(proc->kernelStack);
	kfree(proc->userStack);
	procShrinkMemory(proc, proc->heapSize / PAGE_SIZE);
	freePageTables(proc->vm);
	proc->state = UNUSED;
}

/* proc_load loads the given ELF process image into the given process. */
bool procLoad(ProcessT *proc, char **procImage, int pageCount)
{
	int progHeaderOffset = 0;
	int progHeaderCount = 0;
	int i = 0;

	struct ElfHeader *header = (struct ElfHeader *) procImage[0];
	if (header->type != ELFTYPE_EXECUTABLE)
		return false;

	(void) pageCount;

	progHeaderOffset = header->phoff;
	progHeaderCount = header->phnum;

	for (i = 0; i < progHeaderCount; i++) {
		uint32_t j = 0;
		struct ElfProgramHeader *header = (void *) (procImage[0] + progHeaderOffset);

		/* make enough room for this section */
		while (proc->heapSize < header->vaddr + header->memsz)
			procExpandMemory(proc, 1);

		/* copy the section */
		for (j = 0; j < header->memsz; j++) {
			int vaddr = header->vaddr + j;
			int paddr = resolvePhyAddress(proc->vm, vaddr);
			char *ptr = (char *) P2V(paddr);
			int imageOff = header->off + j;
			*ptr = procImage[imageOff / PAGE_SIZE][imageOff % PAGE_SIZE];
		}

		progHeaderOffset += sizeof(struct ElfProgramHeader);
	}

	proc->entry = (EntryFunctionT) header->entry;
	proc->state = READY;

	memset(proc->context, 0, sizeof(proc->context));
	proc->context[CPSR] = 0x10;
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
