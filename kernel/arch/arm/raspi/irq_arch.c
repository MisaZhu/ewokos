#include <dev/gic.h>
#include <kernel/irq.h>
#include <dev/uart.h>
#include <dev/kdevice.h>
#include <kernel/kernel.h>
#include <kernel/hw_info.h>
#include <mm/mmu.h>
#include <kstring.h>
#include <arch.h>
#include <dev/sd.h>

/* memory mapping for the prime interrupt controller */
#define PIC (_mmio_base + 0xB200)
#define PIC_INT_UART0 (25+64)
#define PIC_INT_SDC (30)


#define IRQ_IS_BASIC(x) ((x) >= 64 )
#define IRQ_IS_GPU2(x) ((x) >= 32 && (x) < 64 )
#define IRQ_IS_GPU1(x) ((x) < 32 )
#define IRQ_IS_PENDING(regs, num) ((IRQ_IS_BASIC(num) && ((1 << (num-64)) & regs->irq_basic_pending)) || \
		(IRQ_IS_GPU2(num) && ((1 << (num-32)) & regs->irq_gpu_pending2)) || \
		(IRQ_IS_GPU1(num) && ((1 << (num)) & regs->irq_gpu_pending1)))

static pic_regs_t* _pic;
static void enable_irq(uint32_t id) {
	uint32_t irq_pos;
	if(IRQ_IS_BASIC(id)) {
		irq_pos           = id - 64;
		_pic->irq_basic_enable |= (1 << irq_pos);
	}
	else if (IRQ_IS_GPU2(id)) {
		irq_pos           = id - 32;
		_pic->irq_gpu_enable2 |= (1 << irq_pos);
	}
	else if (IRQ_IS_GPU1(id)) {
		irq_pos           = id;
		_pic->irq_gpu_enable1 |= (1 << irq_pos);
	}
}

void irq_arch_init(void) {
	_pic = (pic_regs_t*)PIC;
}


inline void gic_set_irqs(uint32_t irqs) {
	(void)irqs;
  if((irqs & IRQ_TIMER0) != 0) {
  	enable_irq(64);
	}

	/*
  if((irqs & IRQ_SDC) != 0) {
		enable_irq((PIC_INT_SDC + 32)); //pic->irq_enable2 EMMC int routing enabled.
		uint32_t offset = GPU_INTERRUPTS_ROUTING - get_hw_info()->phy_mmio_base;
		uint32_t vbase = _mmio_base+offset;
		put32(vbase, 0x00);
	}
  if((irqs & IRQ_UART0) != 0)  
		enable_irq(PIC_INT_UART0);
	*/
}

inline uint32_t gic_get_irqs(void) {
	uint32_t ret = 0;
	if(IRQ_IS_PENDING(_pic, 64)) {
		ret |= IRQ_TIMER0;
	}

	/*if(uart_ready_to_recv() == 0) {
		ret |= IRQ_UART0;
	}
	ret |= IRQ_SDC;
	*/
	return ret;
}
