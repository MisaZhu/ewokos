#include <ewoksys/interrupt.h>
#include <ewoksys/syscall.h>

int32_t sys_interrupt_setup(uint32_t interrupt, uint32_t irq_raw, interrupt_handler_t handler, uint32_t data) {
	return syscall3(SYS_INTR_SETUP, interrupt | (irq_raw << 16), (int32_t)handler, data);
}

void sys_interrupt_end(void) {
	syscall0(SYS_INTR_END);
}
