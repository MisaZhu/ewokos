#include <ewoksys/interrupt.h>
#include <ewoksys/syscall.h>

int32_t sys_interrupt_setup(uint32_t interrupt, interrupt_handler_t handler, uint32_t data) {
	if(interrupt >= SYS_INT_MAX)
		return -1;

	return syscall3(SYS_INTR_SETUP, interrupt, (int32_t)handler, data);
}

void sys_interrupt_end(void) {
	syscall0(SYS_INTR_END);
}
