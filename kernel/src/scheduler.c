#include <proc.h>
#include <scheduler.h>

void schedule(void) {
	if(_current_proc->state == READY) { //current process ready to run
		_current_proc->state = RUNNING;
		proc_start(_current_proc);
		return;
	}

	//current process is runing, switch to next one.
	process_t* head_proc = _current_proc;
	process_t* proc = _current_proc->next;

	while(head_proc != proc) {
		if(proc->state == READY)  {
			if(head_proc->state == RUNNING) //current process running.
				head_proc->state = READY;
			proc->state = RUNNING; //run next one.
			proc_start(proc);
			return;
		}
		else if(proc->state == TERMINATED)  {
			process_t* tmp = proc;
			proc = proc->next;
			proc_free(tmp);
		}
		else 
			proc = proc->next;
	}
}

