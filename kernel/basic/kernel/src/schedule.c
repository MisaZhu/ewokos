#include <kernel/proc.h>
#include <kernel/system.h>
#include <kprintf.h>
#include <stddef.h>

int32_t spin;

void schedule(context_t* ctx) {
	uint32_t core_id = get_core_id();
	if(core_id > 0) { 
		smp_lock(&spin);
		printf("core %d schedule\n", core_id);
		smp_unlock(&spin);
		return; // TODO
	}

	proc_t* next = proc_get_next_ready();
	if(next != NULL) {
		next->info.state = RUNNING;
		proc_switch(ctx, next, false);
	}
}
