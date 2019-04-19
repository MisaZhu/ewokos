#include <irq.h>
#include <mm/mmu.h>
#include <kstring.h>
#include <types.h>
#include <timer.h>
#include <dev/uart.h>
#include <hardware.h>

/* interrupt controller register offsets */
#define PIC_STATUS      2
#define PIC_INT_ENABLE  5
#define PIC_INT_DISABLE 9

#define PIC_BASE_OFF 0xB200

#define CORE0_TIMER__irqCNTL 0x40000040

void routing_core0cntv_to_core0irq(void) {
	uint32_t offset = CORE0_TIMER__irqCNTL-get_mmio_base_phy();
	uint32_t vbase = MMIO_BASE+offset;
	mmio_write(vbase, 0x08);
}

#define CORE0__irq_SOURCE 0x40000060
uint32_t read_core0timer_pending(void) {
	uint32_t tmp;
	uint32_t offset = CORE0__irq_SOURCE - get_mmio_base_phy();
	uint32_t vbase = MMIO_BASE+offset;
	tmp = mmio_read(vbase);
	return tmp;
}

extern uint32_t _timer_frq;

void write_cntv_tval(uint32_t val) {
	__asm__ volatile ("mcr p15, 0, %0, c14, c3, 0" :: "r"(val) );
	return;
}

void irq_init() {
	routing_core0cntv_to_core0irq();
}

void enable_irq(uint32_t line) {
	volatile uint32_t* vpic = (volatile uint32_t*)(MMIO_BASE+PIC_BASE_OFF);
  vpic[PIC_INT_ENABLE] |= (1<<line);
}

void disable_irq(uint32_t line) {
	volatile uint32_t* vpic = (volatile uint32_t*)(MMIO_BASE+PIC_BASE_OFF);
  vpic[PIC_INT_DISABLE] |= (1<<line);
}

#define PINT_UART0 (1 << 25)

void irq_handle() {
	volatile uint32_t* vpic = (volatile uint32_t*)(MMIO_BASE+PIC_BASE_OFF);
	volatile uint32_t pic_status = vpic[PIC_STATUS];

	if (read_core0timer_pending() & 0x08 ) {
		write_cntv_tval(_timer_frq);    // clear cntv interrupt and set next 1/100 sec timer.
		timer_handle();
  }
	if((pic_status & PINT_UART0) != 0) {
		uart_handle();
	}
}

