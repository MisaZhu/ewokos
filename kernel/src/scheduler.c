#include <proc.h>
#include <timer.h>
#include <irq.h>
#include <hardware.h>

void schedule(void) {
	if(_currentProcess == NULL)
		return;

	if(_currentProcess->state == READY) {
		_currentProcess->state = RUNNING;
		procStart(_currentProcess);
		return;
	}

	ProcessT* proc = _currentProcess->next;
	while(_currentProcess != proc) {
		if(proc->state == READY)  {
			if(_currentProcess->state == RUNNING)
				_currentProcess->state = READY;
			proc->state = RUNNING;
			procStart(proc);
			return;
		}
		proc = proc->next;
	}
}

void handleTimer(void)
{
	timerClearInterrupt();
	schedule();
}

#define SCHEDULE_TIME 10 /*0.01 sec*/

void schedulerInit(void)
{
	timerSetInterval(SCHEDULE_TIME);
	registerInterruptHandler(getTimerIrq(), handleTimer);
}

