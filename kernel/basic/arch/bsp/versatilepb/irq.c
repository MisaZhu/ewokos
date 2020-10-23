#include <dev/dev.h>
#include <dev/gic.h>
#include <kernel/irq.h>
#include <mm/mmu.h>
#include "arch.h"

/* memory mapping for the prime interrupt controller */
#define PIC (_mmio_base + 0x00140000)
#define PIC_INT_TIMER0 (1 << 4)


void irq_arch_init(void) {
}

void gic_set_irqs(uint32_t irqs) {
	pic_regs_t* pic = (pic_regs_t*)(PIC);
	
  if((irqs & IRQ_TIMER0) != 0) 
		pic->enable |= PIC_INT_TIMER0;
}

uint32_t gic_get_irqs(void) {
	uint32_t ret = 0;
	pic_regs_t* pic = (pic_regs_t*)(PIC);

  if((pic->status & PIC_INT_TIMER0) != 0) 
		ret |= IRQ_TIMER0;
	return ret;
}
