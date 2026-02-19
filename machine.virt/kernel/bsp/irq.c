#include <kernel/irq.h>
#include <kernel/hw_info.h>
#include <mm/mmu.h>
#include <gic.h>
#include <kprintf.h>
#include "arch.h"

extern uint64_t interrupt_table_start[];
void irq_init_arch(void) {
	gic_init(MMIO_BASE, MMIO_BASE + 0x10000);
	set_vector_table(&interrupt_table_start);
}

void irq_enable_arch(uint32_t irq) {
	if(irq == IRQ_TIMER0)
		irq = 27;
	gic_irq_enable(0, irq);
}

void irq_enable_core_arch(uint32_t core, uint32_t irq) {
	gic_irq_enable(core, irq);
}

inline void irq_clear_core_arch(uint32_t core, uint32_t irq) {

}

inline void irq_clear_arch(uint32_t irq) {

}

void irq_disable_arch(uint32_t irq) {
	if(irq == IRQ_TIMER0)
		irq = 27;
	gic_irq_disable(0, irq);
}

inline uint32_t irq_get_arch(void) {
	uint32_t irqno = gic_get_irq() & 0x3FF;
	return irqno;
}

inline uint32_t irq_get_unified_arch(uint32_t irqno) {
	if(irqno == 27){
		irqno = IRQ_TIMER0;
	}else if(irqno == 0){
		irqno = IRQ_IPI;
	}
	return irqno;
}

inline void irq_eoi_arch(uint32_t irq_raw) {
	gic_eoi(irq_raw);
}

