#include <kernel/proc.h>
#include <kernel/kernel.h>
#include <kernel/system.h>
#include <kernel/core.h>
#include <stddef.h>
#include <kstring.h>

int32_t schedule(context_t* ctx) {
	uint32_t core = get_core_id();
	proc_t* next = proc_get_next_ready();
	if(next == NULL && _cpu_cores[core].halt_pid != 0) {
		next = proc_get(_cpu_cores[core].halt_pid);
	}

	if(next != NULL) {
		next->info.state = RUNNING;
		proc_switch(ctx, next, false);
		return 0;
	}
	
	halt();
	return 0;
}

