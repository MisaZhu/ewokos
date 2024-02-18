#include <ewoksys/interrupt.h>
#include <ewoksys/syscall.h>

typedef struct {
	interrupt_handler_t handler;
	uint32_t data;
} interrupt_data_t;

static void sys_interrupt_handler(uint32_t interrupt, uint32_t data) {
	interrupt_data_t* idata = (interrupt_data_t*)data;
	idata->handler(interrupt, idata->data);
	syscall0(SYS_INTR_END);
}

int32_t sys_interrupt_setup(uint32_t interrupt, uint32_t irq_raw, interrupt_handler_t handler, uint32_t data) {
	interrupt_data_t* idata = (interrupt_data_t*)malloc(sizeof(interrupt_data_t));
	idata->handler = handler;
	idata->data = data;

	return syscall3(SYS_INTR_SETUP, interrupt | (irq_raw << 16), (int32_t)sys_interrupt_handler, (int32_t)idata);
}
