#include <kernel/irq.h>
#include <kernel/kernel.h>
#include <kernel/hw_info.h>
#include <kernel/system.h>
#include "../timer_arch.h"

#define CORE0_IRQ_CNTL_OFFSET    0x40
#define CORE0_IRQ_SOURCE_OFFSET  0x60

#define L1_INTC_BASE			 0xB200
#define IRQ_BASIC_PENDING   (MMIO_BASE + L1_INTC_BASE + 0x00)  // 基本中断挂起寄存器
#define IRQ_PENDING_1       (MMIO_BASE + L1_INTC_BASE + 0x04)  // 中断挂起寄存器1
#define IRQ_PENDING_2       (MMIO_BASE + L1_INTC_BASE + 0x08)  // 中断挂起寄存器2
#define FIQ_CONTROL         (MMIO_BASE + L1_INTC_BASE + 0x0C)  // FIQ控制寄存器
#define ENABLE_IRQS_1       (MMIO_BASE + L1_INTC_BASE + 0x10)  // 使能中断寄存器1
#define ENABLE_IRQS_2       (MMIO_BASE + L1_INTC_BASE + 0x14)  // 使能中断寄存器2
#define ENABLE_BASIC_IRQS   (MMIO_BASE + L1_INTC_BASE + 0x18)  // 使能基本中断寄存器
#define DISABLE_IRQS_1      (MMIO_BASE + L1_INTC_BASE + 0x1C)  // 禁用中断寄存器1
#define DISABLE_IRQS_2      (MMIO_BASE + L1_INTC_BASE + 0x20)  // 禁用中断寄存器2
#define DISABLE_BASIC_IRQS  (MMIO_BASE + L1_INTC_BASE + 0x24)  // 禁用基本中断寄存器

static void routing_core0_irq(void) {
  uint32_t vbase = _sys_info.mmio.v_base + _core_base_offset + CORE0_IRQ_CNTL_OFFSET;
  put32(vbase, 0x08);
}

static uint32_t read_core0_pending(void) {
  uint32_t vbase = _sys_info.mmio.v_base + _core_base_offset + CORE0_IRQ_SOURCE_OFFSET;
  return get32(vbase);
}

void irq_arch_init_pix(void) {
	routing_core0_irq();
	set_vector_table(&interrupt_table_start);
}

uint32_t irq_get_pix(void) {
	uint32_t ret = 0;
	uint32_t pending = read_core0_pending();

	if (pending & 0x08 ) {
		ret = IRQ_TIMER0;
	}
	return ret;
}

void irq_enable_pix(uint32_t irq) {
	if(IRQ_TIMER0 == irq){
		*((uint32_t*)ENABLE_BASIC_IRQS) = (1 << 5);
	}
}

void irq_disable_pix(uint32_t irq) {
	if(IRQ_TIMER0 == irq){
		*((uint32_t*)DISABLE_BASIC_IRQS) = (1 << 5);
	}
}
