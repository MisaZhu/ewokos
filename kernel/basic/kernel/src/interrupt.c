#include <interrupt.h>
#include <kernel/interrupt.h>
#include <kernel/context.h>
#include <kernel/proc.h>
#include <stddef.h>
#include <kstring.h>
#include <kernel/schedule.h>

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

void  interrupt_send(context_t* ctx, int32_t interrupt, uint32_t data) {
	if(_interrupts[interrupt].pid <= 0 || _interrupts[interrupt].entry == 0)
		return;

	proc_t* proc = proc_get(_interrupts[interrupt].pid);
	proc_t* cproc = get_current_proc();
	if(proc == NULL)
		return;
	
	proc->space->interrupt.proc_state = proc->info.state;
	memcpy(&proc->space->interrupt.ctx, &proc->ctx, sizeof(context_t));

	proc->ctx.pc = _interrupts[interrupt].entry;
	proc->ctx.gpr[0] = interrupt;
	proc->ctx.gpr[1] = data;
	if(proc != cproc) {
		proc->info.state = RUNNING;
		proc_switch(ctx, proc, true);
	}
	else {
		memcpy(ctx, &proc->space->interrupt.ctx, sizeof(context_t));
	}
}

void interrupt_end(context_t* ctx) {
	proc_t* cproc = get_current_proc();
	cproc->info.state = cproc->space->interrupt.proc_state;
	memcpy(ctx, &cproc->space->interrupt.ctx, sizeof(context_t));
	schedule(ctx);
}

