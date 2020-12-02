#include <dev/gic.h>
#include <kernel/irq.h>
#include <kernel/kernel.h>
#include <kernel/hw_info.h>
#include <dev/dev.h>
#include <bcm283x/ipi.h>
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

inline void gic_set_irqs(uint32_t irqs) {
	(void)irqs;
}

void __write_cntv_tval(uint32_t); 

inline uint32_t gic_get_irqs(void) {
	uint32_t ret = 0;
	uint32_t pending = read_core0_pending();

	if (pending & 0x08 ) {
		ret |= IRQ_TIMER0;
		__write_cntv_tval(_timer_frq); 
	}
	return ret;
}

void ipi_enable(uint32_t core_id) {
	bcm283x_ipi_enable(core_id);
}

void ipi_clear(uint32_t core_id) {
	bcm283x_ipi_clear(core_id);
}

void ipi_send(uint32_t core_id) {
	bcm283x_ipi_send(core_id);
}
