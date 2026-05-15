#include <kernel/proc.h>
#include <kernel/kernel.h>
#include <kprintf.h>
#include <kernel/system.h>
#include <kernel/core.h>
#include <stddef.h>

int32_t schedule(context_t* ctx) {
	proc_zombie_funeral();

	uint32_t core = get_core_id();
	proc_t* cproc = get_current_proc();
	proc_t* idle_proc = NULL;
	if(_cpu_cores[core].idle_proc != NULL) {
		/*halt proc supposed to be suspended with WAIT status.
			until there is no proc ready to run.
		*/
		idle_proc = _cpu_cores[core].idle_proc;
		idle_proc->info.state = WAIT;
		idle_proc->info.wait_for = 0;
	}
	
	proc_t* next = proc_get_next_ready();
	if(next == NULL && idle_proc != NULL) {
		next = idle_proc;
	}

	if(next != NULL) {
		next->info.state = RUNNING;
		proc_switch(ctx, next, false);
		return 0;
	}
	
	printf("Panic: none proc to be scheduled!\n");
	halt();
	return 0;
}
