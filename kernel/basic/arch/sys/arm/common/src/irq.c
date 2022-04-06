#include <kernel/irq.h>

inline void irq_enable_cpsr(uint32_t* cpsr) {
	*cpsr &= (~0x80);
}

inline void irq_disable_cpsr(uint32_t* cpsr) {
	*cpsr |= 0x80;
}
