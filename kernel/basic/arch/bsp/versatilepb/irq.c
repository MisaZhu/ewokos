#include <dev/dev.h>
#include <kernel/irq.h>
#include <mm/mmu.h>
#include "arch.h"

#define UART0 ((volatile uint32_t*)(_mmio_base+0x001f1000))
/* serial port register offsets */
#define UART_DATA        0x00
#define UART_FLAGS       0x18
#define UART_INT_ENABLE  0x0e
#define UART_INT_TARGET  0x0f
#define UART_INT_CLEAR   0x11

/* memory mapping for the prime interrupt controller */
#define PIC (_mmio_base + 0x00140000)
#define PIC_INT_TIMER0 (1 << 4)
#define PIC_INT_UART0  (1 << 12)


void irq_arch_init(void) {
}

void irq_enable(uint32_t irqs) {
	pic_regs_t* pic = (pic_regs_t*)(PIC);
	
  if((irqs & IRQ_TIMER0) != 0) 
		pic->enable |= PIC_INT_TIMER0;
  if((irqs & IRQ_UART0) != 0) 
		pic->enable |= PIC_INT_UART0;
}

void irq_disable(uint32_t irqs) {
	pic_regs_t* pic = (pic_regs_t*)(PIC);
	
  if((irqs & IRQ_TIMER0) != 0) 
		pic->enable &= (~PIC_INT_TIMER0);
  if((irqs & IRQ_UART0) != 0) 
		pic->enable &= (~PIC_INT_UART0);
}

uint32_t irq_gets(void) {
	uint32_t ret = 0;
	pic_regs_t* pic = (pic_regs_t*)(PIC);

  if((pic->status & PIC_INT_TIMER0) != 0) 
		ret |= IRQ_TIMER0;
  if((pic->status & PIC_INT_UART0) != 0) 
		ret |= IRQ_UART0;
	return ret;
}

