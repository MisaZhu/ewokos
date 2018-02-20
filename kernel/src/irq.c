#include <irq.h>

/* interrupt handler vector */
static InterruptHandlerT _interruptHandlers[IRQ_COUNT] = {0};

/*
 * register_interrupt_handler enables the given interrupt line, and assigns the
 * given handler function to that line.
 */
void registerInterruptHandler(int line, InterruptHandlerT handler)
{
	enableIRQ(line);
	_interruptHandlers[line] = handler;
}

/*
 * dispatch_interrupts is called when an interrupt occurs. This function finds
 * the line in which interrupt happened and then triggers the correct handler
 * for that interrupt. In case of no handler for that interrupt, this function
 * does nothing.
 */
void handleIRQ(void)
{
	InterruptHandlerT handler = 0;
	bool pendingIRQs[IRQ_COUNT];
	getPendingIRQs(pendingIRQs);

	for (int i = 0; i < IRQ_COUNT; i++) {
		if (pendingIRQs[i]) {
			handler = _interruptHandlers[i];
			if (handler != 0)
				handler();
		}
	}
}

