#include <kernel/proc.h>
#include <kernel/system.h>
#include <stddef.h>

static int32_t _spin = 0;

static void _schedule(context_t* ctx) {
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
