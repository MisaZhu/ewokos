#include <kernel/irq.h>
#include <kernel/kernel.h>
#include <kernel/hw_info.h>
#include "timer_arch.h"

#define CORE0_IRQ_CNTL_OFFSET    0x40
#define CORE0_IRQ_SOURCE_OFFSET  0x60

static void routing_core0_irq(void) {
  uint32_t vbase = _sys_info.mmio.v_base + _core_base_offset + CORE0_IRQ_CNTL_OFFSET;
  put32(vbase, 0x08);
}

static uint32_t read_core0_pending(void) {
  uint32_t vbase = _sys_info.mmio.v_base + _core_base_offset + CORE0_IRQ_SOURCE_OFFSET;
  return get32(vbase);
}

extern uint64_t interrupt_table_start[];
void irq_arch_init(void) {
	routing_core0_irq();
	uint64_t vector_table_addr = interrupt_table_start;
    __asm(
        "msr vbar_el1, %0"
        :
        : "r" (vector_table_addr)
        : "memory"
    );
}

inline uint32_t irq_get(void) {
	uint32_t ret = 0;
	uint32_t pending = read_core0_pending();

	if (pending & 0x08 ) {
		ret = IRQ_TIMER0;
	}
	return ret;
}

inline void irq_enable(uint32_t irq) {
	(void)irq;
}

void irq_disable(uint32_t irq) {
	(void)irq;
}
