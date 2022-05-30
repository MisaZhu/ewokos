#include <kernel/irq.h>
#include <kernel/kernel.h>
#include <kernel/hw_info.h>
#include "timer_arch.h"

//#include <sys/arm/common/v7/gic.h>
extern void gic_init(void);
extern void gic_irq_enable(int irqno);
void irq_arch_init(void) {
	gic_init();
}

inline uint32_t irq_gets(void) {
	//uint32_t ret = 0;
	return IRQ_TIMER0;
}

inline void irq_enable(uint32_t irqs) {
	//gic_irq_enable(26);
	gic_irq_enable(27);
	(void)irqs;
}

void irq_disable(uint32_t irqs) {
	(void)irqs;
}

