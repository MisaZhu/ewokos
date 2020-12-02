#include <kernel/proc.h>
#include <kernel/system.h>
#include <kprintf.h>
#include <stddef.h>

static int32_t _spin = 0;

static void _schedule(context_t* ctx) {
	uint32_t core_id = get_core_id();
	if(core_id > 0) { 
		printf("core %d schedule\n", core_id);
		return; // TODO
	}

	proc_t* next = proc_get_next_ready();
	if(next != NULL) {
		next->info.state = RUNNING;
		proc_switch(ctx, next, false);
	}
}

void schedule(context_t* ctx) {
	smp_lock(&_spin);
	_schedule(ctx);
	smp_unlock(&_spin);
}
