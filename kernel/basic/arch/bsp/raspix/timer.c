#include <dev/mmio.h>
#include <dev/timer.h>
#include <kernel/irq.h>
#include <basic_math.h>
#include "timer_arch.h"

uint32_t _timer_tval  = 0;

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

void __write_cntv_tval(uint32_t);
void __enable_cntv(void);

static inline uint32_t read_cntfrq(void) {
  uint32_t val;
  __asm__ volatile ("mrc p15, 0, %0, c14, c0, 0" : "=r"(val) );
  return val;
}

inline void write_cntv_tval(uint32_t tval) {
	__asm__ volatile ("mcr p15, 0, %0, c14, c3, 0" :: "r"(tval));
}

static inline void enable_cntv(void) {
	__asm__ volatile ("mcr p15, 0, %0, c14, c3, 1" :: "r"(1));
}

#define MIN_FREQ 4096 
void timer_set_interval(uint32_t id, uint32_t times_per_sec) {
	(void)id;
	if (times_per_sec < MIN_FREQ)
		times_per_sec = MIN_FREQ;
	_timer_tval = read_cntfrq() / times_per_sec /20;
	write_cntv_tval(_timer_tval);
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
