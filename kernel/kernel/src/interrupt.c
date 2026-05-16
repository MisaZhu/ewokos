#include <interrupt.h>
#include <kernel/interrupt.h>
#include <kernel/context.h>
#include <kernel/proc.h>
#include <stddef.h>
#include <kstring.h>
#include <kprintf.h>
#include <kernel/schedule.h>
#include <kernel/irq.h>
#include <kernel/system.h>
#include <mm/mmu.h>
#include <mm/kmalloc.h>

typedef struct interrupt_st {
	int32_t  uuid;
	ewokos_addr_t entry;
	ewokos_addr_t data;
} interrupt_handler_t;

typedef struct {
	interrupt_handler_t handler;
	uint32_t irq;
} interrupt_item_t;

typedef struct {
	int32_t pid;
	uint32_t uuid;
} interrupt_queue_item_t;

static interrupt_item_t _interrupts[SYS_INT_MAX];

void interrupt_init(void) {
	memset(&_interrupts, 0, sizeof(interrupt_item_t)*SYS_INT_MAX);	
}

static inline interrupt_item_t* get_interrupt(uint32_t irq) {
	for(uint32_t i=0; i<SYS_INT_MAX; i++) {
		interrupt_item_t* item = &_interrupts[i];
		if(item->irq == irq)
			return item;
	}
	return NULL;
}

int32_t interrupt_setup(proc_t* cproc, uint32_t interrupt, ewokos_addr_t entry, ewokos_addr_t data) {
	interrupt_item_t* item = get_interrupt(interrupt);
	
	if(entry == 0) {//unregister interrupt
		if(item == NULL)
			return -1;
		//irq_disable_arch(interrupt); //TODO
		memset(item, 0, sizeof(interrupt_item_t));
	}
	else { //register interrupt
		if(item == NULL)
			item = get_interrupt(0);
		if(item == NULL)
			return -1;
		item->irq = interrupt;
		item->handler.uuid = cproc->info.uuid;
		item->handler.entry = entry;
		item->handler.data = data;
		irq_enable_core_arch(cproc->info.core, interrupt); //TODO
	}
	return 0;
}

static int32_t interrupt_send_raw(context_t* ctx, uint32_t interrupt,  interrupt_handler_t* intr) {
	if(intr->uuid <= 0 || intr->entry == 0) {
		ctx->gpr[0] = -1;
		return -1;
	}

	proc_t* cproc = get_current_proc();
	proc_t* proc = proc_get_by_uuid(intr->uuid);
	if(proc == NULL) {
		ctx->gpr[0] = -1;
		return -1;
	}

	if(proc->space->interrupt.state != INTR_STATE_IDLE) {
		if(interrupt != IRQ_SOFT)
			printf("inter err re-entre: intr:%d, pid:%d core:%d\n", interrupt, proc == NULL ? -1:proc->info.pid, proc->info.core);
		ctx->gpr[0] = -1;
		return -1;
	}	

	if(interrupt != IRQ_SOFT)
		irq_disable_arch(interrupt);

	/*
	if(proc->ipc_res.state != IPC_IDLE) {
		//printf("inter err ipc req: intr:%d, pid:%d\n", interrupt, proc == NULL ? -1:proc->info.pid);
		ctx->gpr[0] = -1;
		return -1;
	}

	ipc_task_t* ipc = proc_ipc_get_task(proc); 
	if(ipc != NULL) { //target proc still busy on ipc service task
		printf("inter err ipc svr: intr:%d, pid:%d\n", interrupt, proc->info.pid);
		ctx->gpr[0] = -1;
		return -1;
	}
	*/

	proc_save_state(proc, &proc->space->interrupt.saved_state, &proc->space->interrupt.saved_ipc_res);
	proc->space->interrupt.interrupt = interrupt;
	proc->space->interrupt.entry = intr->entry;
	proc->space->interrupt.data = intr->data;
	proc->space->interrupt.state = INTR_STATE_START;
	proc->space->interrupt.counter = 0;
	proc->space->interrupt.restore_pending = 0;

	//if(interrupt != IRQ_SOFT)
		//irq_disable_cpsr(&proc->ctx); //disable interrupt on proc

	proc_switch_multi_core(ctx, proc, cproc->info.core);
	return 0;
}

int32_t  interrupt_send(context_t* ctx, uint32_t interrupt) {
	interrupt_item_t* item = get_interrupt(interrupt);
	if(item == NULL)
		return -1;

	int32_t res = interrupt_send_raw(ctx, interrupt, &item->handler);
	return res;
}

int32_t  interrupt_soft_send(context_t* ctx, int32_t to_pid, ewokos_addr_t entry, ewokos_addr_t data) {
	interrupt_handler_t intr;
	proc_t* proc = proc_get(to_pid);
	intr.entry = entry;
	intr.uuid = proc->info.uuid;
	intr.data = data;
	return interrupt_send_raw(ctx, IRQ_SOFT, &intr);
}

int32_t proc_interrupt_wait(context_t* ctx, struct st_proc* serv_proc, proc_t* proc) {
	if(serv_proc == NULL || proc == NULL) {
		return -1;
	}

	interrupt_queue_item_t* item = (interrupt_queue_item_t*)kmalloc(sizeof(interrupt_queue_item_t));
	if(item == NULL) {
		return -1;
	}

	item->pid = proc->info.pid;
	item->uuid = proc->info.uuid;
	queue_push(&serv_proc->space->interrupt.wait_queue, item);
	proc_block(ctx, proc);
	return 0;
}

proc_t* proc_interrupt_wakeup(struct st_proc* serv_proc) {
	if(serv_proc == NULL) {
		return NULL;
	}

	interrupt_queue_item_t* item = (interrupt_queue_item_t*)queue_pop(&serv_proc->space->interrupt.wait_queue);
	if(item == NULL) {
		return NULL;
	}

	proc_t* proc = proc_get(item->pid);
	if(proc == NULL || proc->info.uuid != item->uuid) {
		kfree(item);
		return NULL;
	}

	kfree(item);
	proc_wakeup(proc);
	return proc;
}

void proc_interrupt_wakeup_all(struct st_proc* serv_proc) {
	if(serv_proc == NULL) {
		return;
	}

	while(true) {
		interrupt_queue_item_t* item = (interrupt_queue_item_t*)queue_pop(&serv_proc->space->interrupt.wait_queue);
		if(item == NULL) {
			break;
		}

		proc_t* proc = proc_get(item->pid);
		if(proc == NULL || proc->info.uuid != item->uuid) {
			kfree(item);
			continue;
		}

		kfree(item);
		proc_wakeup(proc);
	}
}

void interrupt_end(context_t* ctx) {
	proc_t* cproc = get_current_proc();
	uint32_t interrupt = cproc->space->interrupt.interrupt;

	cproc->space->interrupt.state = INTR_STATE_IDLE;
	cproc->space->interrupt.counter = 0;
	cproc->space->interrupt.restore_pending = 0;

	if(cproc->info.state == UNUSED || cproc->info.state == ZOMBIE) {
		proc_interrupt_wakeup(cproc);
		schedule(ctx);
		return;
	}

	proc_restore_state(ctx, cproc, &cproc->space->interrupt.saved_state, &cproc->space->interrupt.saved_ipc_res);
	if(cproc->info.state == READY || cproc->info.state == RUNNING) {
		proc_ready(cproc);
	}

	if(interrupt != IRQ_SOFT) {
		irq_enable_arch(interrupt);
	}
	cproc->space->interrupt.entry = 0;
	cproc->space->interrupt.data = 0;
	cproc->space->interrupt.interrupt = 0;
	proc_interrupt_wakeup(cproc);
	schedule(ctx);
}
