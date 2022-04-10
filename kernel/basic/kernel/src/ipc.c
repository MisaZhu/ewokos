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
	_ipc_uid++;
	cproc->space->ipc_server.uid = _ipc_uid;
	cproc->space->ipc_server.entry = entry;
	cproc->space->ipc_server.extra_data = extra_data;
	cproc->space->ipc_server.flags = flags;
	
	if(cproc->space->small_stack == 0) {
		uint32_t page = (uint32_t)kalloc4k();
		map_page(cproc->space->vm, page, V2P(page), AP_RW_RW, 0);
		cproc->space->small_stack = page;
		flush_tlb();
	}
	return 0;
}

int32_t proc_ipc_call(context_t* ctx, proc_t* proc, ipc_t *ipc) {
	proc_t* cproc = get_current_proc();
	if(proc == NULL || proc->space->ipc_server.entry == 0 || ipc == NULL)
		return -1;

	proc_block_on(proc->info.pid, (uint32_t)ipc);

	proc->space->ipc_server.ipc = (uint32_t)ipc;
	proc->space->ipc_server.state = proc->info.state;
	proc->space->ipc_server.block_by = proc->info.block_by;

	if(proc->info.core == cproc->info.core) {
		proc_switch(ctx, proc, true);
	}
	else {
		proc_ready(proc);
		schedule(ctx);
#ifdef KERNEL_SMP
		ipi_send(proc->info.core);
#endif
	}
	return 0;
}

ipc_t* proc_ipc_req(proc_t* proc) {
	return &proc->ipc_ctx;
}

void proc_ipc_close(ipc_t* ipc) {
	proto_clear(&ipc->data);
	memset(ipc, 0, sizeof(ipc_t));
	ipc->state = IPC_IDLE;
}
