#include <proc.h>
#include <timer.h>
#include <irq.h>
#include <hardware.h>

static int roundRobinIndex;

void schedule(void)
{
	ProcessT *proc;
	proc = NULL;
	for (int i = 0; i < PROCESS_COUNT_MAX; i++) {
		proc = &_processTable[roundRobinIndex];

		roundRobinIndex++;
		if (roundRobinIndex == PROCESS_COUNT_MAX) {
			roundRobinIndex = 0;
		}

		if (proc->state == READY) {
			_currentProcess = proc;
			procStart(proc);
		}
	}

	/* no process? DEAD!!! */
	while(1);
}

void handleTimer(void)
{
	timerClearInterrupt();
	schedule();
}

#define SCHEDULE_TIME 100000 /*0.1 sec*/

void schedulerInit(void)
{
	roundRobinIndex = 0;
	timerSetInterval(SCHEDULE_TIME);
	registerInterruptHandler(getTimerIrq(), handleTimer);
}

