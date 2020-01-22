#include <kernel/proc.h>
#include <kernel/system.h>
#include <kprintf.h>

void schedule(context_t* ctx) {
	proc_t* next = proc_get_next_ready();
	if(next != NULL) {
		proc_switch(ctx, next);
	}
}
