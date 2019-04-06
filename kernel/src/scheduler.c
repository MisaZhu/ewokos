#include <proc.h>
#include <timer.h>
#include <irq.h>
#include <hardware.h>
#include <scheduler.h>
#include <printk.h>

static bool _schedule_enabled = true;

void schedule(void) {
	if(!_schedule_enabled) {
		printk("schedule_disabled.\n");
		return;
	}

	if(_current_proc->state == READY) { //current process ready to run
		_current_proc->state = RUNNING;
		proc_start(_current_proc);
	}
	else if(_current_proc->state == RUNNING) { //current process running.
		_current_proc->state = READY;
	}

	//current process is runing, switch to next one.
	process_t* head_proc = _current_proc;
	process_t* proc = _current_proc->next;

	while(head_proc != proc) {
		if(proc->state == READY)  {
			proc->state = RUNNING; //run next one.
			proc_start(proc);
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

static void handle_timer(void) {
	timer_clear_interrupt();
	schedule();
}

#define SCHEDULE_TIME 10 /*0.01 sec*/

void scheduler_init(void) {
	timer_set_interval(SCHEDULE_TIME);
	register_interrupt_handler(get_timer_irq(), handle_timer);
}

void disable_schedule() {
	_schedule_enabled = false;
}

void enable_schedule() {
	_schedule_enabled = true;
}
