#include <ewoksys/interrupt.h>
#include <ewoksys/syscall.h>

static void sys_interrupt_handler(uint32_t interrupt, uint32_t data) {
	interrupt_handler_t* idata = (interrupt_handler_t*)data;
	idata->handler(interrupt, idata->data);
	syscall0(SYS_INTR_END);
}

int32_t sys_interrupt_setup(uint32_t interrupt, interrupt_handler_t* handler) {
	if(handler == NULL) {
		return syscall3(SYS_INTR_SETUP, interrupt, 0, 0);
	}

	return syscall3(SYS_INTR_SETUP, interrupt, (int32_t)sys_interrupt_handler, (int32_t)handler);
}
