#include <kernel/irq.h>
#include <kernel/hw_info.h>
#include <mm/mmu.h>
#include <gic.h>
#include <kprintf.h>
#include "arch.h"

extern uint64_t interrupt_table_start[];
void irq_arch_init(void) {
	gic_init(MMIO_BASE, MMIO_BASE + 0x10000);
	set_vector_table(&interrupt_table_start);
}

void irq_enable(uint32_t irq) {
	if(irq == IRQ_TIMER0)
		irq = 27;
	gic_irq_enable(0, irq);
}

void irq_enable_core(uint32_t core, uint32_t irq) {
	gic_irq_enable(core, irq);
}


void irq_disable(uint32_t irq) {
	if(irq == IRQ_TIMER0)
		irq = 27;
	gic_irq_disable(0, irq);
}

inline uint32_t irq_get(void) {
	uint32_t irqno = gic_get_irq();
	if(irqno == 27){
		irqno = IRQ_TIMER0;
	}else if(irqno == 0){
		irqno = IRQ_IPI;
	}
	return irqno;
}

