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
#include <mm/kalloc.h>

typedef struct {
	int32_t  pid;
	uint32_t entry;
} interrupt_t;

static interrupt_t _interrupts[SYS_INT_MAX];

void interrupt_init(void) {
	memset(&_interrupts, 0, sizeof(interrupt_t)*SYS_INT_MAX);	
}

int32_t interrupt_setup(proc_t* cproc, uint32_t interrupt, uint32_t entry) {
	_interrupts[interrupt].entry = entry;
	_interrupts[interrupt].pid = cproc->info.pid;

	cproc->space->interrupt.stack = proc_stack_alloc(cproc);
	return 0;
}

void  interrupt_send(context_t* ctx, uint32_t interrupt) {
	if(_interrupts[interrupt].pid <= 0 || _interrupts[interrupt].entry == 0)
		return;

	proc_t* proc = proc_get(_interrupts[interrupt].pid);
	proc_t* cproc = get_current_proc();
	if(proc == NULL)
		return;
	
	proc->info.state = RUNNING; //TODO
	proc_save_state(proc, &proc->space->interrupt.saved_state);
	proc->space->interrupt.interrupt = interrupt;
	proc->space->interrupt.entry = _interrupts[interrupt].entry;
	proc->space->interrupt.do_switch = true;

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

void interrupt_end(context_t* ctx) {
	proc_t* cproc = get_current_proc();

	proc_restore_state(ctx, cproc, &cproc->space->interrupt.saved_state);
	irq_enable_cpsr(&cproc->ctx.cpsr); //enable interrupt on proc
	schedule(ctx);
}

