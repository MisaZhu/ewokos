#include <kernel/irq.h>
#include <kernel/kernel.h>
#include <kernel/hw_info.h>
#include <dev/mmio.h>
#include "timer_arch.h"

#define CORE0_TIMER__irqCNTL 0x40000040
#define CORE0__irq_SOURCE    0x40000060

static void routing_core0_irq(void) {
  uint32_t offset = CORE0_TIMER__irqCNTL - _sys_info.mmio.phy_base;
  uint32_t vbase = _mmio_base+offset;
  put32(vbase, 0x08);
}

static uint32_t read_core0_pending(void) {
  uint32_t tmp;
  uint32_t offset = CORE0__irq_SOURCE -  _sys_info.mmio.phy_base;
  uint32_t vbase = _mmio_base+offset;
  tmp = get32(vbase);
  return tmp;
}

void irq_arch_init(void) {
	routing_core0_irq();
}

inline uint32_t irq_gets(void) {
	uint32_t ret = 0;
	uint32_t pending = read_core0_pending();

	if (pending & 0x08 ) {
		ret |= IRQ_TIMER0;
		write_cntv_tval(_timer_tval); 
	}
	return ret;
}

inline void irq_enable(uint32_t irqs) {
	(void)irqs;
}

void irq_disable(uint32_t irqs) {
	(void)irqs;
}

