#include <kernel/proc.h>
#include <stddef.h>

int32_t schedule(context_t* ctx) {
	proc_t* next = proc_get_next_ready();
	if(next != NULL) {
		next->info.state = RUNNING;
		proc_switch(ctx, next, false);
		return 0;
	}
	return -1;
}

