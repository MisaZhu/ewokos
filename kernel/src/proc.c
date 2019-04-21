#include <mm/mmu.h>
#include <mm/kmalloc.h>
#include <mm/kalloc.h>
#include <proc.h>
#include <kernel.h>
#include <kstring.h>
#include <system.h>
#include <types.h>
#include <dev/uart.h>
#include <elf.h>
#include <kernel.h>
#include <kserv.h>
#include <scheduler.h>
#include <printk.h>
#include <timer.h>
#include <mm/shm.h>

process_t _process_table[PROCESS_COUNT_MAX];

__attribute__((__aligned__(PAGE_DIR_SIZE)))
static page_dir_entry_t _processVM[PROCESS_COUNT_MAX][PAGE_DIR_NUM];

process_t* _current_proc = NULL;
static int32_t _p_proc_lock = 0;

/* proc_init initializes the process sub-system. */
void proc_init(void) {
	for (int i = 0; i < PROCESS_COUNT_MAX; i++)
		_process_table[i].state = UNUSED;
	_current_proc = NULL;
}

/* proc_exapnad_memory expands the heap size of the given process. */
bool proc_expand_mem(void *p, int page_num, bool read_only) {
	CRIT_IN(_p_proc_lock)
	process_t* proc = (process_t*)p;	
	for (int i = 0; i < page_num; i++) {
		char *page = kalloc();
		if(page == NULL) {
			printk("proc expand failed!! free mem size: (%x)\n", get_free_mem_size());
			proc_shrink_mem(proc, i);
			CRIT_OUT(_p_proc_lock)
			return false;
		}
		memset(page, 0, PAGE_SIZE);

		if(read_only) 
			map_page(proc->space->vm, 
					proc->space->heap_size,
					V2P(page),
					AP_RW_R);
		else
			map_page(proc->space->vm, 
					proc->space->heap_size,
					V2P(page),
					AP_RW_RW);
		proc->space->heap_size += PAGE_SIZE;
	}
	CRIT_OUT(_p_proc_lock)
	return true;
}

bool proc_expand(void *p, int page_num) {
	return proc_expand_mem(p, page_num, false);
}

/* proc_shrink_memory shrinks the heap size of the given process. */
void proc_shrink_mem(void* p, int page_num) {
	CRIT_IN(_p_proc_lock)
	process_t* proc = (process_t*)p;	
	for (int i = 0; i < page_num; i++) {
		uint32_t virtual_addr = proc->space->heap_size - PAGE_SIZE;
		uint32_t physical_addr = resolve_phy_address(proc->space->vm, virtual_addr);

		page_table_entry_t * pge = get_page_table_entry(proc->space->vm, virtual_addr);
		if(pge->permissions != AP_RW_R || proc->entry != NULL) { //copied pages or proc loaded from elf.
			//get the kernel address for kalloc/kfree
			uint32_t kernel_addr = P2V(physical_addr);
			kfree((void *) kernel_addr);
		}

		unmap_page(proc->space->vm, virtual_addr);
		proc->space->heap_size -= PAGE_SIZE;
		if (proc->space->heap_size == 0) {
			break;
		}
	}
	CRIT_OUT(_p_proc_lock)
}

static void* proc_get_mem_tail(void* p) {
	process_t* proc = (process_t*)p;	
	return (void*)proc->space->heap_size;
}

static void insert(process_t* proc) { /*insert to ready process loop*/
	if(_current_proc == NULL)
		_current_proc = proc;
	proc->next = _current_proc;
	proc->prev = _current_proc->prev;
	_current_proc->prev->next = proc;
	_current_proc->prev = proc;
}

static void remove(process_t* proc) { /*remove from ready process loop*/
	proc->next->prev = proc->prev;
	proc->prev->next = proc->next;

	if(proc->next == proc) //only one left.
		_current_proc = NULL;
	else
		_current_proc = proc->next;

	proc->next = NULL;
	proc->prev = NULL;
}

static void proc_init_space(process_t* proc) {
	page_dir_entry_t *vm = _processVM[proc->pid];
	set_kernel_vm(vm);

	proc->space = (process_space_t*)km_alloc(sizeof(process_space_t));
	proc->space->vm = vm;
	proc->space->heap_size = 0;
	proc->space->malloc_man.arg = (void*)proc;
	proc->space->malloc_man.mHead = 0;
	proc->space->malloc_man.mTail = 0;
	proc->space->malloc_man.expand = proc_expand;
	proc->space->malloc_man.shrink = NULL; //TODO
	//proc->space->malloc_man.shrink = proc_shrink_mem;
	proc->space->malloc_man.get_mem_tail = proc_get_mem_tail;
	memset(&proc->space->files, 0, sizeof(proc_file_t)*FILE_MAX);
}

static inline  uint32_t proc_get_user_stack(process_t* proc) {
	uint32_t ret;
	if(proc->type == TYPE_THREAD)
		ret = USER_STACK_BOTTOM - proc->pid*PAGE_SIZE;
	else 
		ret = USER_STACK_BOTTOM;
	return ret;
}

/* proc_creates allocates a new process and returns it. */
process_t *proc_create(uint32_t type) {
	CRIT_IN(_p_proc_lock)
	int index = -1;
	for (int i = 0; i < PROCESS_COUNT_MAX; i++) {
		if (_process_table[i].state == UNUSED) {
			index = i;
			break;
		}
	}
	if (index < 0) {
		CRIT_OUT(_p_proc_lock)
		return NULL;
	}

	process_t *proc = &_process_table[index];
	memset(proc, 0, sizeof(process_t));
	proc->pid = index;
	proc->type = type;
	if(type == TYPE_THREAD)
		proc->space = _current_proc->space;
	else
		proc_init_space(proc);

	/*
	char *kernelStack = kalloc();
	map_page(vm, 
		KERNEL_STACK_BOTTOM,
		V2P(kernelStack),
		AP_RW_R);
	proc->kernelStack = kernelStack;
	*/

	char *user_stack = kalloc();
	map_page(proc->space->vm,
		proc_get_user_stack(proc),
		V2P(user_stack),
		AP_RW_RW);

	proc->user_stack = user_stack;
	proc->wait_pid = -1;
	proc->father_pid = 0;
	proc->owner = 0;
	proc->cmd[0] = 0;
	strcpy(proc->pwd, "/");
	proc->state = CREATED;
	cpu_tick(&proc->start_sec, NULL);
	
	insert(proc);
	CRIT_OUT(_p_proc_lock)
	return proc;
}

inline int32_t *get_current_context(void) {
	return _current_proc->context;
}

static void proc_free_space(process_t *proc) {
	/*free file info*/
	for(uint32_t i=0; i<FILE_MAX; i++) {
		k_file_t* kf = proc->space->files[i].kf;
		if(kf != NULL) {
			kf_unref(kf, proc->space->files[i].flags); //unref the kernel file table.
		}
	}

	kserv_unreg(proc);
	ipc_close_all(proc->pid);
	shm_proc_free(proc->pid);
	proc_shrink_mem(proc, proc->space->heap_size / PAGE_SIZE);
	free_page_tables(proc->space->vm);
	km_free(proc->space);
}

/* proc_free frees all resources allocated by proc. */
void proc_free(process_t *proc) {
	CRIT_IN(_p_proc_lock)	
	proc->state = UNUSED;
	//kfree(proc->kernelStack);
	kfree(proc->user_stack);

	if(proc->type == TYPE_THREAD)
		proc->space = NULL;
	else
		proc_free_space(proc);
	remove(proc);
	CRIT_OUT(_p_proc_lock)	
}

#define MODE_MASK 0x1f
#define MODE_USER 0x10
#define DIS_INT (1<<7)

static inline uint32_t cpsrUser() {
    uint32_t val;
    __asm__ volatile("mrs %[v], cpsr": [v]"=r" (val)::);
    val &= ~MODE_MASK;
    val |= MODE_USER;
    val &= ~DIS_INT;
    return val;
}

/* proc_load loads the given ELF process image into the given process. */
bool proc_load(process_t *proc, const char *pimg, uint32_t img_size) {
	uint32_t prog_header_offset = 0;
	uint32_t progHeaderCount = 0;
	uint32_t i = 0;
	
	CRIT_IN(_p_proc_lock)
	/*move to kernel memory to save procImg, coz orignal address will be covered.*/
	int32_t shmid = shm_alloc(img_size);
	char* proc_image = NULL;
	if(_current_proc != NULL)
		proc_image = (char*)shm_proc_map(_current_proc->pid, shmid);
	else
		proc_image = (char*)shm_raw(shmid);
	if(proc_image == NULL) {
		CRIT_OUT(_p_proc_lock)
		return false;
	}
	memcpy(proc_image, pimg, img_size);
	proc_shrink_mem(proc, proc->space->heap_size/PAGE_SIZE);

	/*read elf format from saved proc image*/
	struct elf_header *header = (struct elf_header *) proc_image;
	if (header->type != ELFTYPE_EXECUTABLE) {
		shm_free(shmid);
		CRIT_OUT(_p_proc_lock)
		return false;
	}
	prog_header_offset = header->phoff;
	progHeaderCount = header->phnum;

	for (i = 0; i < progHeaderCount; i++) {
		uint32_t j = 0;
		struct elf_program_header *header = (void *) (proc_image + prog_header_offset);
		bool read_only = false;
		if((header->flags & 0x2) == 0)
			read_only = true;

		/* make enough room for this section */
		while (proc->space->heap_size < header->vaddr + header->memsz) {
			if(!proc_expand_mem(proc, 1, read_only)) {
				printk("Panic: proc expand memory failed!!(%s: %d)\n", proc->cmd, proc->pid);
				shm_free(shmid);
				CRIT_OUT(_p_proc_lock)
				return false;
			}
		}
		/* copy the section from kernel to proc mem space*/
		uint32_t hvaddr = header->vaddr;
		uint32_t hoff = header->off;
		for (j = 0; j < header->memsz; j++) {
			uint32_t vaddr = hvaddr + j; /*vaddr in elf (proc vaddr)*/
			uint32_t paddr = resolve_phy_address(proc->space->vm, vaddr); /*trans to phyaddr by proc's page dir*/
			uint32_t vkaddr = P2V(paddr); /*trans the phyaddr to vaddr now in kernel page dir*/
			/*copy from elf to vaddrKernel(=phyaddr=vaddrProc=vaddrElf)*/
			
			uint32_t image_off = hoff + j;
			*(char*)vkaddr = proc_image[image_off];
		}
		prog_header_offset += sizeof(struct elf_program_header);
	}
	shm_free(shmid);

	proc->space->malloc_man.mHead = 0;
	proc->space->malloc_man.mTail = 0;
	proc->entry = (entry_function_t) header->entry;
	proc->state = READY;
	memset(proc->context, 0, sizeof(proc->context));
	proc->context[CPSR] = cpsrUser(); //CPSR 0x10 for user mode
	proc->context[RESTART_ADDR] = (int) proc->entry;
	proc->context[SP] = proc_get_user_stack(proc) + PAGE_SIZE;
	CRIT_OUT(_p_proc_lock)
	return true;
}

/* proc_start starts running the given process. */
void proc_start(process_t *proc) {
	_current_proc = proc;
	__set_translation_table_base((uint32_t) V2P(proc->space->vm));
	/* clear TLB */
	__asm__ volatile("mov R4, #0");
	__asm__ volatile("MCR p15, 0, R4, c8, c7, 0");
	__switch_to_context(proc->context);
}

process_t* proc_get(int pid) {
	if(pid < 0 || pid >= PROCESS_COUNT_MAX)
		return NULL;
	CRIT_IN(_p_proc_lock)
	process_t* proc = &_process_table[pid];
	if(proc->state == UNUSED) {
		CRIT_OUT(_p_proc_lock)
		return NULL;
	}
	CRIT_OUT(_p_proc_lock)
	return proc;
}

static int32_t proc_clone(process_t* child, process_t* parent) {
	CRIT_IN(_p_proc_lock)
	uint32_t pages = parent->space->heap_size / PAGE_SIZE;
	if((parent->space->heap_size % PAGE_SIZE) != 0)
		pages++;

	uint32_t p;
	uint32_t i;
	for(p=0; p<pages; ++p) {
		uint32_t v_addr = (p * PAGE_SIZE);
		page_table_entry_t * pge = get_page_table_entry(parent->space->vm, v_addr);
		if(pge->permissions == AP_RW_R) {
			uint32_t phy_page_addr = resolve_phy_address(parent->space->vm, v_addr);
			map_page(child->space->vm, 
					child->space->heap_size,
					phy_page_addr,
					AP_RW_R);
			child->space->heap_size += PAGE_SIZE;
		}
		else {
			if(!proc_expand_mem(child, 1, false)) {
				printk("Panic: kfork expand memory failed!!(%s: %d)\n", parent->cmd, parent->pid);
				CRIT_OUT(_p_proc_lock)
					return -1;
			}
			/* copy parent's memory to child's memory */
			for (i=0; i<PAGE_SIZE; ++i) {
				uint32_t v_addr = (p * PAGE_SIZE) + i;
				uint32_t child_phy_addr = resolve_phy_address(child->space->vm, v_addr);
				char *child_ptr = (char *) P2V(child_phy_addr);
				char *parent_ptr = (char *) v_addr;
				*child_ptr = *parent_ptr;
			}
		}
	}

	/*pmalloc list*/
	child->space->malloc_man = parent->space->malloc_man;
	child->space->malloc_man.arg = child;
	/*file info*/
	for(i=0; i<FILE_MAX; i++) {
		k_file_t* kf = parent->space->files[i].kf;
		if(kf != NULL) {
			child->space->files[i].kf = kf;
			child->space->files[i].flags = parent->space->files[i].flags;
			child->space->files[i].seek = parent->space->files[i].seek;
			kf_ref(kf, child->space->files[i].flags); //ref the kernel file table.
		}
	}
	/* copy parent's stack to child's stack */
	//memcpy(child->kernelStack, parent->kernelStack, PAGE_SIZE);
	memcpy(child->user_stack, parent->user_stack, PAGE_SIZE);
	/* copy parent's context to child's context */
	memcpy(child->context, parent->context, sizeof(child->context));
	child->entry = NULL; //cloned process.
	CRIT_OUT(_p_proc_lock)
	return 0;
}

process_t* kfork(uint32_t type) {
	CRIT_IN(_p_proc_lock)

	process_t *child = NULL;
	process_t *parent = _current_proc;

	child = proc_create(type);
	if(type != TYPE_THREAD) {
		if(proc_clone(child, parent) != 0) {
			printk("panic: kfork clone failed!!(%s: %d)\n", parent->cmd, parent->pid);
			proc_exit(child);
			CRIT_OUT(_p_proc_lock)
			return NULL;
		}
	}
	else {
		memset(child->context, 0, sizeof(child->context));
		child->context[SP] = proc_get_user_stack(child) + PAGE_SIZE;
		child->context[CPSR] = parent->context[CPSR];
	}
	/* set return value of fork in child to 0 */
	child->context[R0] = 0;
	/* child is ready to run */
	child->state = READY;
	/*set father*/
	child->father_pid = parent->pid;
	/*same owner*/
	child->owner = parent->owner;
	/*cmd*/
	strcpy(child->cmd, parent->cmd);
	/*pwd*/
	strcpy(child->pwd, parent->pwd);
	CRIT_OUT(_p_proc_lock)
	return child;
}

static void proc_terminate(process_t* proc) {
	proc->state = TERMINATED;
	process_t *p = proc->next;
	while(p != proc) {
		/*terminate forked from this proc*/
		if(p->father_pid == proc->pid) {
			proc_terminate(p);
		}
		else if (p->state == SLEEPING &&
				p->wait_pid == proc->pid) {
			p->wait_pid = -1;
			p->state = READY;
		}
		p = p->next;
	}
}

void proc_exit(process_t* proc) {
	if (proc == NULL)
		return;
	CRIT_IN(_p_proc_lock)	
	__set_translation_table_base((uint32_t) V2P(_kernel_vm));
	__asm__ volatile("ldr sp, = _init_stack");
	__asm__ volatile("add sp, #4096");

	proc_terminate(proc);
	CRIT_OUT(_p_proc_lock)	
	schedule();
	return;
}

void proc_sleep(int32_t by) {
	CRIT_IN(_p_proc_lock)
	_current_proc->state = SLEEPING;
	_current_proc->slept_by = by;
	CRIT_OUT(_p_proc_lock)
}

void proc_wake_pid(int32_t pid) {
	CRIT_IN(_p_proc_lock)
	process_t* proc = proc_get(pid);
	if(proc->state == SLEEPING) {
		proc->state = READY;
		proc->slept_by = 0;
	}
	CRIT_OUT(_p_proc_lock)
}

void proc_wake(int32_t by) {
	CRIT_IN(_p_proc_lock)
	process_t *p = _current_proc->next;
	while(p != _current_proc) {
		if (p->state == SLEEPING &&
				(p->slept_by < 0 || p->slept_by == by)) {
			p->slept_by = 0;
			p->state = READY;
		}
		p = p->next;
	}
	CRIT_OUT(_p_proc_lock)
}

void _abort_entry() {
	printk("process %d: '%s' abort!!!\n", _current_proc->pid, _current_proc->cmd);
	proc_exit(_current_proc);
}

void* pmalloc(uint32_t size) {
	if(_current_proc == NULL || size == 0)
		return NULL;
	CRIT_IN(_p_proc_lock)
	void* ret = trunk_malloc(&_current_proc->space->malloc_man, size);
	CRIT_OUT(_p_proc_lock)
	return ret;
}

void pfree(void* p) {
	CRIT_IN(_p_proc_lock)
	trunk_free(&_current_proc->space->malloc_man, p);
	CRIT_OUT(_p_proc_lock)
}
