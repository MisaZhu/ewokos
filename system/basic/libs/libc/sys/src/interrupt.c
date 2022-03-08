#include <sys/interrupt.h>
#include <sys/syscall.h>

int32_t sys_interrupt_setup(uint32_t interrupt, interrupt_handler_t handler) {
	if(interrupt >= SYS_INT_MAX)
		return -1;

	return syscall2(SYS_INTR_SETUP, interrupt, handler);
}

void sys_interrupt_end(void) {
	syscall0(SYS_INTR_END);
}
