#include <kernel/irq.h>
#include <kernel/kernel.h>
#include <kernel/hw_info.h>
#include "timer_arch.h"
#include <gic.h>

void irq_arch_init(void) {
	gic_init();
}

inline uint32_t irq_gets(void) {
	return IRQ_TIMER0;
}

inline void irq_enable(uint32_t irqs) {
	gic_irq_enable(27);
	(void)irqs;
}

void irq_disable(uint32_t irqs) {
	(void)irqs;
}

