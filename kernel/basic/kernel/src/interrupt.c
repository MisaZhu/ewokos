#include <interrupt.h>
#include <kernel/interrupt.h>
#include <kernel/context.h>
#include <kernel/proc.h>
#include <stddef.h>
#include <kstring.h>
#include <kernel/schedule.h>
#include <kernel/irq.h>

typedef struct {
	int32_t pid;
	uint32_t entry;
} interrupt_t;

static interrupt_t _interrupts[SYS_INT_MAX];

void interrupt_init(void) {
	memset(&_interrupts, 0, sizeof(interrupt_t)*SYS_INT_MAX);	
}

void interrupt_setup(int32_t pid, uint32_t interrupt, uint32_t entry) {
	_interrupts[interrupt].entry = entry;
	_interrupts[interrupt].pid = pid;
}

void  interrupt_send(context_t* ctx, uint32_t interrupt) {
	if(_interrupts[interrupt].pid <= 0 || _interrupts[interrupt].entry == 0)
		return;

	proc_t* proc = proc_get(_interrupts[interrupt].pid);
	proc_t* cproc = get_current_proc();
	if(proc == NULL)
		return;
	
	proc->space->interrupt.proc_state = proc->info.state;
	memcpy(&proc->space->interrupt.ctx, &proc->ctx, sizeof(context_t));

	proc->space->interrupt.interrupt = interrupt;
	proc->space->interrupt.entry = _interrupts[interrupt].entry;
	irq_disable_cpsr(&proc->ctx.cpsr); //disable interrupt on proc
	if(proc->info.core == cproc->info.core) {
		proc->info.state = RUNNING;
		proc_switch(ctx, proc, true);
	}
	else {
		proc_ready(proc);
		schedule(ctx);
	}
}

void interrupt_end(context_t* ctx) {
	proc_t* cproc = get_current_proc();
	cproc->info.state = cproc->space->interrupt.proc_state;
	memcpy(ctx, &cproc->space->interrupt.ctx, sizeof(context_t));
	irq_enable_cpsr(&cproc->ctx.cpsr); //enable interrupt on proc
	schedule(ctx);
}

