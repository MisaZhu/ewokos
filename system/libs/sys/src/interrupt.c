#include <sys/interrupt.h>
#include <sys/syscall.h>

int proc_interrupt_register(uint32_t int_id) {
	return syscall1(SYS_PROC_USINT_REGISTER, (int32_t)int_id);
}

int get_proc_interrupt_pid(uint32_t int_id) {
	return syscall1(SYS_GET_USINT_PID, (int32_t)int_id);
}

int proc_interrupt_unregister(uint32_t int_id) {
	return syscall1(SYS_PROC_USINT_UNREGISTER, (int32_t)int_id);
}

