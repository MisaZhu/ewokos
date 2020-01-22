#include <dev/gic.h>
#include <kernel/irq.h>
#include <mm/mmu.h>
#include <arch.h>

/* memory mapping for the prime interrupt controller */
#define PIC (_mmio_base + 0x00140000)
/* memory mapping for the slave interrupt controller */
#define SIC (_mmio_base + 0x00003000)

#define PIC_INT_TIMER0 (1 << 4)
#define PIC_INT_UART0 (1 << 12)
#define PIC_INT_SIC (1 << 31)

#define SIC_INT_KEY (1 << 3)
#define SIC_INT_MOUSE (1 << 4)
#define SIC_INT_SDC (1 << 22)

void irq_arch_init(void) {
}

void gic_set_irqs(uint32_t irqs) {
	pic_regs_t* pic = (pic_regs_t*)(PIC);
	sic_regs_t* sic = (sic_regs_t*)(SIC);
	
	pic->enable |= PIC_INT_SIC;

  if((irqs & IRQ_TIMER0) != 0) 
		pic->enable |= PIC_INT_TIMER0;
  if((irqs & IRQ_UART0) != 0) 
		pic->enable |= PIC_INT_UART0;

  if((irqs & IRQ_KEY) != 0) 
		sic->enable |= SIC_INT_KEY;
  if((irqs & IRQ_MOUSE) != 0) 
		sic->enable |= SIC_INT_MOUSE;
  if((irqs & IRQ_SDC) != 0) 
		sic->enable |= SIC_INT_SDC;
}

uint32_t gic_get_irqs(void) {
	uint32_t ret = 0;
	pic_regs_t* pic = (pic_regs_t*)(PIC);
	sic_regs_t* sic = (sic_regs_t*)(SIC);

  if((pic->status & PIC_INT_TIMER0) != 0) 
		ret |= IRQ_TIMER0;
  if((pic->status & PIC_INT_UART0) != 0)
		ret |= IRQ_UART0;

  if((pic->status & PIC_INT_SIC) != 0) {
    if((sic->status & SIC_INT_KEY) != 0) 
			ret |= IRQ_KEY;
    if((sic->status & SIC_INT_MOUSE) != 0) 
			ret |= IRQ_MOUSE;
    if((sic->status & SIC_INT_SDC) != 0)
			ret |= IRQ_SDC;
  }
	return ret;
}
