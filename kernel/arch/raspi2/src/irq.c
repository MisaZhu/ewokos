#include <irq.h>
#include <hardware.h>
#include <mm/mmu.h>
#include <kstring.h>
#include <types.h>

/* interrupt controller register offsets */
//#define PIC_STATUS     0x81
//#define PIC_INT_ENABLE 0x84
#define PIC_STATUS      0
#define PIC_INT_ENABLE  6
#define PIC_INT_DISABLE 9

#define PIC_BASE_OFF 0xB200

#define CORE0_TIMER_IRQCNTL 0x40000040

void routing_core0cntv_to_core0irq(void)
{
	uint32_t base = MMIO_BASE+CORE0_TIMER_IRQCNTL-getMMIOBasePhy();
	mmio_write(base, 0x08);
}

#define CORE0_IRQ_SOURCE 0x40000060
uint32_t read_core0timer_pending(void)
{
	uint32_t tmp;
	uint32_t base = MMIO_BASE+CORE0_IRQ_SOURCE-getMMIOBasePhy();
	tmp = mmio_read(base);
	return tmp;
}

static uint32_t cntfrq = 0x4000;

void enable_cntv(void)
{
	uint32_t cntv_ctl;
	cntv_ctl = 1;
	__asm__ volatile ("mcr p15, 0, %0, c14, c3, 1" :: "r"(cntv_ctl) ); // write CNTV_CTL
}

void disable_cntv(void)
{
	uint32_t cntv_ctl;
	cntv_ctl = 0;
	__asm__ volatile ("mcr p15, 0, %0, c14, c3, 1" :: "r"(cntv_ctl) ); // write CNTV_CTL
}
/*
uint32_t read_cntv_tval(void)
{
	uint32_t val;
	__asm__ volatile ("mrc p15, 0, %0, c14, c3, 0" : "=r"(val) );
	return val;
}

uint32_t read_cntfrq(void)
{
	uint32_t val;
	__asm__ volatile ("mrc p15, 0, %0, c14, c0, 0" : "=r"(val) );
	return val;
}
*/

void write_cntv_tval(uint32_t val)
{
	__asm__ volatile ("mcr p15, 0, %0, c14, c3, 0" :: "r"(val) );
	return;
}

void irqInit() {
	//cntfrq = read_cntfrq();
	write_cntv_tval(cntfrq);    // clear cntv interrupt and set next 1 sec timer.
	routing_core0cntv_to_core0irq();
	enable_cntv(); 
}

void enableIRQ(uint32_t line) {
	volatile uint32_t* vpic = (volatile uint32_t*)(MMIO_BASE+PIC_BASE_OFF);
  vpic[PIC_INT_ENABLE] = (1<<line);
}

void disableIRQ(uint32_t line) {
	volatile uint32_t* vpic = (volatile uint32_t*)(MMIO_BASE+PIC_BASE_OFF);
  vpic[PIC_INT_DISABLE] = (1<<line);
}

void getPendingIRQs(bool *result) {
	volatile uint32_t* vpic = (volatile uint32_t*)(MMIO_BASE+PIC_BASE_OFF);
	volatile uint32_t irqStatus = vpic[PIC_STATUS];
	memset(result, 0, IRQ_COUNT * sizeof(bool));

	if (read_core0timer_pending() & 0x08 ) {
		write_cntv_tval(cntfrq);    // clear cntv interrupt and set next 1sec timer.
		result[getTimerIrq()] = true;
		return;
  }

	for (uint32_t i = 0; i < 32; i++)
		if (irqStatus & (1u << i))
			result[i] = true;
}

