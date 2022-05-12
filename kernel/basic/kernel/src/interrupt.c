#include <interrupt.h>
#include <kernel/interrupt.h>
#include <kernel/context.h>
#include <kernel/proc.h>
#include <stddef.h>
#include <kstring.h>
#include <kernel/schedule.h>
#include <kernel/irq.h>
#include <kernel/system.h>
#include <mm/mmu.h>
#include <mm/kmalloc.h>

typedef struct interrupt_st {
	int32_t  pid;
	uint32_t entry;
	struct   interrupt_st* next;
} interrupt_t;

typedef struct {
	interrupt_t* head;
	interrupt_t* it;
} interrupt_item_t;

static interrupt_item_t _interrupts[SYS_INT_MAX];

void interrupt_init(void) {
	memset(&_interrupts, 0, sizeof(interrupt_item_t)*SYS_INT_MAX);	
}

int32_t interrupt_setup(proc_t* cproc, uint32_t interrupt, uint32_t entry) {
	if(interrupt >= SYS_INT_MAX)
		return -1;

	interrupt_t* intr = (interrupt_t*)kmalloc(sizeof(interrupt_t));
	intr->pid = cproc->info.pid;
	intr->entry = entry;
	intr->next = NULL;

	if(_interrupts[interrupt].head != NULL) {
		intr->next = _interrupts[interrupt].head;
	}
	_interrupts[interrupt].head = intr;

	cproc->space->interrupt.stack = proc_stack_alloc(cproc);
	return 0;
}

static void  interrupt_send_raw(context_t* ctx, uint32_t interrupt,  interrupt_t* intr) {
	if(intr->pid <= 0 || intr->entry == 0)
		return;

	proc_t* proc = proc_get(intr->pid);
	proc_t* cproc = get_current_proc();
	if(proc == NULL)
		return;
	
	proc_save_state(proc, &proc->space->interrupt.saved_state);
	proc->space->interrupt.interrupt = interrupt;
	proc->space->interrupt.entry = intr->entry;
	proc->space->interrupt.do_switch = true;
	if(interrupt != SYS_INT_SOFT)
		irq_disable_cpsr(&proc->ctx.cpsr); //disable interrupt on proc

	if(proc->info.core == cproc->info.core) {
		proc->info.state = RUNNING;
		proc_switch(ctx, proc, true);
	}
	else {
		proc_ready(proc);
#ifdef KERNEL_SMP
		ipi_send(proc->info.core);
#endif
	}
}

static interrupt_t* fetch_next(uint32_t interrupt) {
	interrupt_t* intr = _interrupts[interrupt].it;
	if(intr != NULL)
		_interrupts[interrupt].it = intr->next;
	return intr;
}

void  interrupt_send(context_t* ctx, uint32_t interrupt) {
	if(interrupt >= SYS_INT_MAX)
		return;
	_interrupts[interrupt].it = _interrupts[interrupt].head;
	interrupt_t* intr = fetch_next(interrupt);
	if(intr == NULL)
		return;
	interrupt_send_raw(ctx, interrupt, intr);
}

void interrupt_end(context_t* ctx) {
	proc_t* cproc = get_current_proc();

	uint32_t interrupt = cproc->space->interrupt.interrupt;
	proc_restore_state(ctx, cproc, &cproc->space->interrupt.saved_state);
	if(cproc->info.state == READY)
		proc_ready(cproc);

	if(interrupt != SYS_INT_SOFT)
		irq_enable_cpsr(&cproc->ctx.cpsr); //enable interrupt on proc

	interrupt_t* intr = fetch_next(interrupt);
	if(intr != NULL)
		interrupt_send_raw(ctx, interrupt, intr);
	else
		schedule(ctx);
}
