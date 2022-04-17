#include <kernel/proc.h>
#include <kernel/kernel.h>
#include <kprintf.h>
#include <kernel/system.h>
#include <kernel/core.h>
#include <stddef.h>
#include <kstring.h>

int32_t schedule(context_t* ctx) {
	proc_zombie_funeral();

	uint32_t core = get_core_id();
	proc_t* halt_proc = NULL;
	if(_cpu_cores[core].halt_pid != 0) {
		/*halt proc supposed to be suspended with WAIT status.
			until there is no proc ready to run.
		*/
		halt_proc = proc_get(_cpu_cores[core].halt_pid);
		halt_proc->info.state = WAIT;
		halt_proc->info.wait_for = 0;
	}
	
	proc_t* next = proc_get_next_ready();
	if(next == NULL && halt_proc != NULL) { //if no proc ready, switch to halt proc
		next = halt_proc;
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

