#include <kernel/irq.h>
#include <kernel/kernel.h>
#include <kernel/system.h>
#include <kernel/hw_info.h>
#include "../timer_arch.h"

static uint32_t irq_enable_flag = 0;
void irq_arch_init_pi4(void) {
    irq_enable_flag = 0;
	gic_init(MMIO_BASE + 0x1841000, MMIO_BASE + 0x1842000);
    for(int i = 0; i < 1022; i++){
        gic_irq_disable(0, i);
    }
	set_vector_table(&interrupt_table_start);
}

uint32_t irq_get_pi4(void) {
	int ack = gic_get_irq();
    int irqno = ack & 0x3FF;

    if(irqno == 27){
        return IRQ_TIMER0;
    }else if(irqno == 0){
        return IRQ_IPI;
    }
    return 0;
}

void irq_enable_pi4(int core, uint32_t irq) {
    if(irq & irq_enable_flag)
        return;

	if(irq == IRQ_TIMER0){
        gic_irq_enable(core, 27);
        irq_enable_flag |= irq;
    }
}

void irq_disable_pi4(uint32_t irq) {
	(void)irq;
}
