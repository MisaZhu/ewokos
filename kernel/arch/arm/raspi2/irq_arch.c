#include <dev/gic.h>
#include <kernel/irq.h>
#include <dev/kdevice.h>
#include <kernel/kernel.h>
#include <kernel/hw_info.h>
#include <mm/mmu.h>
#include <kstring.h>
#include <arch.h>
#include "timer_arch.h"
#include <dev/sd.h>

/* memory mapping for the prime interrupt controller */
#define PIC (_mmio_base + 0xB200)
#define PIC_INT_UART0 (25+64)
#define PIC_INT_SDC (30)

static pic_regs_t* _pic;

#define IRQ_IS_BASIC(x) ((x) >= 64 )
#define IRQ_IS_GPU2(x) ((x) >= 32 && (x) < 64 )
#define IRQ_IS_GPU1(x) ((x) < 32 )

/*
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
*/

#define CORE0_TIMER__irqCNTL 0x40000040
#define GPU_INTERRUPTS_ROUTING 0x4000000C

static void routing_core0_irq(void) {
  uint32_t offset = CORE0_TIMER__irqCNTL - get_hw_info()->phy_mmio_base;
  uint32_t vbase = _mmio_base+offset;
  put32(vbase, 0x08);
}

#define CORE0__irq_SOURCE 0x40000060
static uint32_t read_core0_pending(void) {
  uint32_t tmp;
  uint32_t offset = CORE0__irq_SOURCE -  get_hw_info()->phy_mmio_base;
  uint32_t vbase = _mmio_base+offset;
  tmp = get32(vbase);
  return tmp;
}

void irq_arch_init(void) {
	routing_core0_irq();
	_pic = (pic_regs_t*)(PIC);
}

inline void gic_set_irqs(uint32_t irqs) {
	(void)irqs;
	/*
  if((irqs & IRQ_SDC) != 0) {
		enable_irq((PIC_INT_SDC + 32)); //pic->irq_enable2 EMMC int routing enabled.
		uint32_t offset = GPU_INTERRUPTS_ROUTING - get_hw_info()->phy_mmio_base;
		uint32_t vbase = _mmio_base+offset;
		put32(vbase, 0x00);
	}
	*/
}

inline uint32_t gic_get_irqs(void) {
	uint32_t ret = 0;
	uint32_t pending = read_core0_pending();

	if (pending & 0x08 ) {
		ret |= IRQ_TIMER0;
		write_cntv_tval(_timer_frq); 
	}

	/*if (pending & (1 << 8)) { //GPU_INT
		if(_pic->irq_basic_pending & (1 << 9)) { //pending2
			if(_pic->irq_gpu_pending2 & (1 << PIC_INT_SDC)) { //sdc int
				ret |= IRQ_SDC;
			}
		}
	}
	*/
	return ret;
}
