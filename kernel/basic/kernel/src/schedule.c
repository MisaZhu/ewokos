#include <kernel/proc.h>
#include <kernel/kernel.h>
#include <kernel/system.h>
#include <stddef.h>
#include <kstring.h>

int32_t schedule(context_t* ctx) {
	proc_t* cproc = get_current_proc();
	proc_t* next = proc_get_next_ready();

	if(cproc != NULL && cproc->info.state != RUNNING) {
		memcpy(&cproc->ctx, ctx, sizeof(context_t));
	}
	
	if(next != NULL) {
		next->info.state = RUNNING;
		proc_switch(ctx, next, false);
		return 0;
	}
	
	kernel_unlock();
	__irq_enable();
	halt();
	return -1;
}

