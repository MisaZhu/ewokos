#include <kernel/proc.h>
#include <stddef.h>

void schedule(context_t* ctx) {
	proc_t* next = proc_get_next_ready();
	if(next != NULL) {
		next->info.state = RUNNING;
		kernel_unlock();
		proc_switch(ctx, next, false);
		return;
	}
}

