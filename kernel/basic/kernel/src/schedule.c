#include <kernel/proc.h>
#include <kernel/kernel.h>
#include <kernel/system.h>
#include <stddef.h>
#include <kstring.h>

int32_t schedule(context_t* ctx) {
	proc_t* cproc = get_current_proc();
	proc_t* next = proc_get_next_ready();
	if(next != NULL) {
		next->info.state = RUNNING;
		proc_switch(ctx, next, false);
		return 0;
	}

	if(cproc != NULL)
		memcpy(&cproc->ctx, ctx, sizeof(context_t));
	
	//no task to be scheduled, do halt
	set_current_proc(NULL);
	set_translation_table_base(V2P((uint32_t)_kernel_vm));
	ctx->pc = (uint32_t)halt;
	ctx->cpsr = 0x13; //svc mode with interrupts enabled
	ctx->sp = 0x100;
	return 0;
}

