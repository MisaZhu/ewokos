#include <kernel/irq.h>

void irq_enable_cpsr(uint32_t* cpsr) {
	*cpsr &= (~0x80);
}

void irq_disable_cpsr(uint32_t* cpsr) {
	*cpsr |= 0x80;
}
