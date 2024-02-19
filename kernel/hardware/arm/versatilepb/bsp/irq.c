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

	if(irq == IRQ_TIMER0) 
		pic->enable |= PIC_INT_TIMER0;
	else if(irq < 31)
		pic->enable |= (1<<irq);
	else if(irq >= 32) {
		pic->enable |= (0x80000000); //sic enabled
		sic->enable |= (1<<(irq-32));
	}
}

void irq_disable(uint32_t irq) {
	pic_regs_t* pic = (pic_regs_t*)(PIC);
	sic_regs_t* sic = (pic_regs_t*)(SIC);
	
	if(irq == IRQ_TIMER0) 
		pic->enable &= (~PIC_INT_TIMER0);
	else if(irq < 31)
		pic->enable &= ~(1<<irq);
	else if(irq >= 32)
		sic->enable &= ~(1<<(irq-32));
}

static inline get_irq_raw(uint32_t irq_status) {
	uint32_t i=0;
	while(i<32) {
		if(((irq_status >> i) & 0x1) != 0)
			break;
		i++;
	}
	if(i >= 32)
		i = 0;
	return i;
}

inline uint32_t irq_get(void) {
	uint32_t ret = 0;
	pic_regs_t* pic = (pic_regs_t*)(PIC);
	sic_regs_t* sic = (pic_regs_t*)(SIC);

	if((pic->status & PIC_INT_TIMER0) != 0) 
		ret = IRQ_TIMER0;
	else {
		if((pic->status & 0x7fffffff) != 0) {
			ret = get_irq_raw(pic->status);
		}
		else if(sic->status != 0) {
			ret = get_irq_raw(sic->status) + 32;
		}
	}
	return ret;
}

