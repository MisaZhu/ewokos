#include <timer.h>
#include <types.h>
#include <mm/mmu.h>
#include <hardware.h>
#include <irq.h>

/*#define ARM_TIMER_OFF 0xB400
#define ARM_TIMER_CTRL_32BIT (1<<1)
#define ARM_TIMER_CTRL_ENABLE (1<<7)
#define ARM_TIMER_CTRL_IRQ_ENABLE (1<<5)
#define ARM_TIMER_CTRL_PRESCALE_256 (2<<2)

#define IRQ_ARM_TIMER_BIT 0

#define TIMER_LOAD    0x00
#define TIMER_CONTROL 0x02
#define TIMER_INTCTL  0x03
#define TIMER_BGLOAD  0x06
*/

uint32_t read_cntfrq(void) {
	uint32_t val;
	__asm__ volatile ("mrc p15, 0, %0, c14, c0, 0" : "=r"(val) );
	return val;
}
uint32_t _timer_frq  = 0;

extern void write_cntv_tval(uint32_t val);

static void enable_cntv(void) {
	uint32_t cntv_ctl;
	cntv_ctl = 1;
	__asm__ volatile ("mcr p15, 0, %0, c14, c3, 1" :: "r"(cntv_ctl) ); // write CNTV_CTL
}

/*static void disable_cntv(void) {
	uint32_t cntv_ctl;
	cntv_ctl = 0;
	__asm__ volatile ("mcr p15, 0, %0, c14, c3, 1" :: "r"(cntv_ctl) ); // write CNTV_CTL
}
*/

void timer_set_interval(uint32_t interval_microsecond) {
	if(interval_microsecond == 0)
		interval_microsecond = 1000;
	_timer_frq = (read_cntfrq() / interval_microsecond);
	write_cntv_tval(_timer_frq); 
	enable_cntv(); 
}

void timer_init() {
}

void timer_clear_interrupt(void) {
}
