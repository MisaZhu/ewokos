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
	int32_t  uuid;
	uint32_t entry;
	uint32_t data;
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

int32_t interrupt_setup(proc_t* cproc, uint32_t interrupt, uint32_t entry, uint32_t data) {
	if(interrupt >= SYS_INT_MAX)
		return -1;

	interrupt_t* intr = (interrupt_t*)kmalloc(sizeof(interrupt_t));
	intr->uuid = cproc->info.uuid;
	intr->entry = entry;
	intr->data = data;
	intr->next = NULL;

	if(_interrupts[interrupt].head != NULL) {
		intr->next = _interrupts[interrupt].head;
	}
	_interrupts[interrupt].head = intr;

	cproc->space->interrupt.stack = proc_stack_alloc(cproc);
	return 0;
}

static int32_t interrupt_send_raw(context_t* ctx, uint32_t interrupt,  interrupt_t* intr) {
	if(intr->uuid <= 0 || intr->entry == 0)
		return -1;

	proc_t* proc = proc_get_by_uuid(intr->uuid);
	proc_t* cproc = get_current_proc();
	if(proc == NULL || proc->space->interrupt.state != INTR_STATE_IDLE)
		return -1;
	ipc_task_t* ipc = proc_ipc_get_task(proc);
	if(ipc != NULL && ipc->state == IPC_BUSY)
		return -1;
	
	proc_save_state(proc, &proc->space->interrupt.saved_state);
	proc->space->interrupt.interrupt = interrupt;
	proc->space->interrupt.entry = intr->entry;
	proc->space->interrupt.data = intr->data;
	proc->space->interrupt.state = INTR_STATE_START;
	if(interrupt != SYS_INT_SOFT)
		irq_disable_cpsr(&proc->ctx.cpsr); //disable interrupt on proc

	proc_switch_multi_core(ctx, proc, cproc->info.core);
	return 0;
}

static interrupt_t* fetch_next(uint32_t interrupt) {
	interrupt_t* intr = _interrupts[interrupt].it;
	if(intr != NULL)
		_interrupts[interrupt].it = intr->next;
	return intr;
}

int32_t  interrupt_send(context_t* ctx, uint32_t interrupt) {
	if(interrupt >= SYS_INT_MAX)
		return -1;
	_interrupts[interrupt].it = _interrupts[interrupt].head;
	interrupt_t* intr = fetch_next(interrupt);
	if(intr == NULL)
		return -1;
	return interrupt_send_raw(ctx, interrupt, intr);
}

int32_t  interrupt_soft_send(context_t* ctx, int32_t to_pid, uint32_t entry, uint32_t data) {
	interrupt_t intr;
	proc_t* proc = proc_get(to_pid);
	intr.entry = entry;
	intr.uuid = proc->info.uuid;
	intr.data = data;
	return interrupt_send_raw(ctx, SYS_INT_SOFT, &intr);
}

void interrupt_end(context_t* ctx) {
	proc_t* cproc = get_current_proc();
	cproc->space->interrupt.state = INTR_STATE_IDLE;
	proc_wakeup(cproc->info.pid, (uint32_t)&cproc->space->interrupt);

	if(cproc->info.state == UNUSED || cproc->info.state == ZOMBIE) {
		schedule(ctx);
		return;
	}

	uint32_t interrupt = cproc->space->interrupt.interrupt;
	proc_restore_state(ctx, cproc, &cproc->space->interrupt.saved_state);

	if(cproc->info.state == READY)
		proc_ready(cproc);

	if(interrupt != SYS_INT_SOFT) {
		irq_enable_cpsr(&cproc->ctx.cpsr); //enable interrupt on proc
		interrupt_t* intr = fetch_next(interrupt);
		if(intr != NULL)
			interrupt_send_raw(ctx, interrupt, intr);
	}
	schedule(ctx);
}
