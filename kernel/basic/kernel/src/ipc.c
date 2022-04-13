#include <kernel/ipc.h>
#include <kernel/proc.h>
#include <kernel/schedule.h>
#include <kernel/system.h>
#include <mm/kalloc.h>
#include <mm/mmu.h>
#include <stddef.h>
#include <kstring.h>
#include <dev/ipi.h>

int32_t proc_ipc_setup(context_t* ctx, uint32_t entry, uint32_t extra_data, uint32_t flags) {
	(void)ctx;
	proc_t* cproc = get_current_proc();
	cproc->space->ipc_server.entry = entry;
	cproc->space->ipc_server.extra_data = extra_data;
	cproc->space->ipc_server.flags = flags;
	
	if(cproc->space->inter_stack == 0) {
		uint32_t page = (uint32_t)kalloc4k();
		map_page(cproc->space->vm, page, V2P(page), AP_RW_RW, 0);
		cproc->space->inter_stack = page;
		flush_tlb();
	}
	return 0;
}

void proc_ipc_task(context_t* ctx, proc_t* serv_proc) {
	proc_t* client_proc = get_current_proc();
	if(serv_proc == NULL ||
			serv_proc->space->ipc_server.entry == 0 ||
			serv_proc->ipc_task.state == IPC_IDLE ||
			serv_proc->ipc_task.uid == 0) {
		ctx->gpr[0] = 0;
		return;
	}
	
	ipc_task_t* ipc = &serv_proc->ipc_task;
	if(ipc->state == IPC_IDLE || ipc->uid == 0) {
		ctx->gpr[0] = 0;
		return;
	}

	ipc->saved_state = serv_proc->info.state;
	ipc->saved_block_by = serv_proc->info.block_by;
	ipc->saved_block_event = serv_proc->block_event;
	ipc->start = true;

	if(serv_proc->info.core == client_proc->info.core) {
		serv_proc->info.state = RUNNING;
		proc_switch(ctx, serv_proc, true);
	}
	else {
		proc_ready(serv_proc);
		schedule(ctx);
#ifdef KERNEL_SMP
		ipi_send(serv_proc->info.core);
#endif
	}
}

uint32_t proc_ipc_req(void) {
	return ++_ipc_uid;
}

void proc_ipc_close(ipc_task_t* ipc) {
	proto_clear(&ipc->data);
	memset(ipc, 0, sizeof(ipc_task_t));
	ipc->state = IPC_IDLE;
}
