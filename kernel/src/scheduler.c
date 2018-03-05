#include <proc.h>
#include <timer.h>
#include <irq.h>
#include <hardware.h>

static int roundRobinIndex;

void schedule(void)
{
	for (int i = 0; i < PROCESS_COUNT_MAX; i++) {
		ProcessT *proc = NULL;

		roundRobinIndex++;
		if (roundRobinIndex == PROCESS_COUNT_MAX)
		{
			roundRobinIndex = 0;
		}

		proc = &_processTable[roundRobinIndex];

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
	registerInterruptHandler(TIMER_IRQ, handleTimer);
}

