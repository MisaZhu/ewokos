#include <kernel/irq.h>
#include <kernel/hw_info.h>
#include <mm/mmu.h>
#include "arch.h"

#define UART0 ((volatile uint32_t*)(_sys_info.mmio.v_base+0x001f1000))
/* serial port register offsets */
#define UART_DATA        0x00
#define UART_FLAGS       0x18
#define UART_INT_ENABLE  0x0e
#define UART_INT_TARGET  0x0f
#define UART_INT_CLEAR   0x11

/* memory mapping for the prime interrupt controller */
#define PIC (_sys_info.mmio.v_base + 0x00140000)
#define SIC (_sys_info.mmio.v_base + 0x00003000)

#define PIC_INT_TIMER0 (1 << 4)
#define PIC_INT_UART0  (1 << 12)


void irq_arch_init(void) {
}

void irq_enable(uint32_t irq) {
	pic_regs_t* pic = (pic_regs_t*)(PIC);
	sic_regs_t* sic = (pic_regs_t*)(SIC);

	uint32_t irq_raw = (irq >> 16) & 0x0000ffff;
	irq = irq  & 0x0000ffff;
	
  if(irq == IRQ_TIMER0) 
		pic->enable |= PIC_INT_TIMER0;
	else if(irq == IRQ_RAW_P)
		pic->enable |= (1<<irq_raw);
	else if(irq == IRQ_RAW_S) {
		pic->enable |= (0x80000000); //sic enabled
		sic->enable |= (1<<irq_raw);
	}
}

void irq_disable(uint32_t irq) {
	pic_regs_t* pic = (pic_regs_t*)(PIC);
	sic_regs_t* sic = (pic_regs_t*)(SIC);

	uint32_t irq_raw = (irq >> 16) & 0x0000ffff;
	irq = irq  & 0x0000ffff;
	
  if(irq == IRQ_TIMER0) 
		pic->enable &= (~PIC_INT_TIMER0);
	else if(irq == IRQ_RAW_P)
		pic->enable &= ~(1<<irq_raw);
	else if(irq == IRQ_RAW_S)
		sic->enable &= ~(1<<irq_raw);
}

inline uint32_t irq_get(uint32_t* raw_irqs) {
	uint32_t ret = IRQ_NONE;
	pic_regs_t* pic = (pic_regs_t*)(PIC);
	sic_regs_t* sic = (pic_regs_t*)(SIC);

  if((pic->status & PIC_INT_TIMER0) != 0) 
		ret = IRQ_TIMER0;
	else {
		if((pic->status & 0x7fffffff) != 0) {
			*raw_irqs = pic->status;
			ret = IRQ_RAW_P;
		}
		else if(sic->status != 0) {
			*raw_irqs = sic->status;
			ret = IRQ_RAW_S;
		}
	}
	return ret;
}

