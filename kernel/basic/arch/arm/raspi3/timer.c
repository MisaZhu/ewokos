#include <mm/mmu.h>
#include <dev/timer.h>
#include <kernel/irq.h>
#include <basic_math.h>
#include "timer_arch.h"

/*#define ARM_TIMER_BASE (_mmio_base + 0xB400)
#define ARM_TIMER_LOAD ARM_TIMER_BASE
#define ARM_TIMER_CTRL ARM_TIMER_BASE+(4*2)
#define ARM_TIMER_CTRL_32BIT (1<<1)
#define ARM_TIMER_CTRL_ENABLE (1<<7)
#define ARM_TIMER_CTRL_IRQ_ENABLE (1<<5)
#define ARM_TIMER_CTRL_PRESCALE_256 (2<<2)

#define PIC_BASE (_mmio_base + 0xB200)
#define PIC_ENABLE_BASIC_IRQ (PIC_BASE+(4*6))
#define IRQ_ARM_TIMER_BIT 0

void timer_set_interval(uint32_t id, uint32_t interval_microsecond) {
	(void)id;
	put32(PIC_ENABLE_BASIC_IRQ, 1 << IRQ_ARM_TIMER_BIT);
	put32(ARM_TIMER_LOAD, interval_microsecond);
	put32(ARM_TIMER_CTRL, ARM_TIMER_CTRL_32BIT |
			ARM_TIMER_CTRL_ENABLE |
			ARM_TIMER_CTRL_IRQ_ENABLE | ARM_TIMER_CTRL_PRESCALE_256);
}
*/

uint32_t read_cntfrq(void) {
  uint32_t val;
  __asm__ volatile ("mrc p15, 0, %0, c14, c0, 0" : "=r"(val) );
  return val;
}

uint32_t _timer_frq  = 0;
void write_cntv_tval(uint32_t val) {
  __asm__ volatile ("mcr p15, 0, %0, c14, c3, 0" :: "r"(val) );
}

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

void timer_set_interval(uint32_t id, uint32_t interval_microsecond) {
	(void)id;
  if(interval_microsecond == 0)
    interval_microsecond = 100;
  _timer_frq = div_u32(read_cntfrq() , (interval_microsecond*10));
  write_cntv_tval(_timer_frq);
  enable_cntv();
}

void timer_clear_interrupt(uint32_t id) {
	(void)id;
}

#define SYSTEM_TIMER_BASE (_mmio_base+0x3000)
#define SYSTEM_TIMER_LOW  0x0004 // System Timer Counter Upper 32 bits
#define SYSTEM_TIMER_HI   0x0008 // System Timer Counter Upper 32 bits

uint64_t timer_read_sys_usec(void) { //read microsec
	uint64_t r = get32(SYSTEM_TIMER_BASE + SYSTEM_TIMER_HI);
	r <<= 32;
	return (r + get32(SYSTEM_TIMER_BASE + SYSTEM_TIMER_LOW));
}
