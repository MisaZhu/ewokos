#include <irq.h>

/* interrupt handler vector */
static interrupt_handler_t _interruptHandlers[IRQ_COUNT] = {0};

/*
 * register_interrupt_handler enables the given interrupt line, and assigns the
 * given handler function to that line.
 */
void register_interrupt_handler(uint32_t line, interrupt_handler_t handler)
{
	enable_irq(line);
	_interruptHandlers[line] = handler;
}

/*
 * dispatch_interrupts is called when an interrupt occurs. This function finds
 * the line in which interrupt happened and then triggers the correct handler
 * for that interrupt. In case of no handler for that interrupt, this function
 * does nothing.
 */
void handle_irq(void)
{
	interrupt_handler_t handler = 0;
	bool pendingIRQs[IRQ_COUNT];
	get_pending_irqs(pendingIRQs);

	for (uint32_t i = 0; i < IRQ_COUNT; i++) {
		if (pendingIRQs[i]) {
			handler = _interruptHandlers[i];
			if (handler != 0)
				handler();
		}
	}
}

