#include <ewoksys/interrupt.h>
#include <ewoksys/syscall.h>

#define SOFT_INTR_RETRY_MAX 256

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

int32_t sys_soft_intr(int32_t pid, uint32_t entry, uint32_t data) {
	int32_t res = 0;
	uint32_t cnt = 0;
	while(cnt < SOFT_INTR_RETRY_MAX) {
		res = syscall3(SYS_SOFT_INT, pid, entry, data);
		if(res != -1)
			break;
		cnt++;
		sleep(0);
	}

	if(res != 0)
		klog("soft interrupt failed (res: %d, from: %d, to: %d)!\n", res, getpid(), pid);
	return res;
}