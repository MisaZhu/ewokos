#include <dev/timer.h>
#include <kernel/irq.h>
#include <kernel/kernel.h>
#include <mm/mmu.h>
#include "../timer_arch.h"

uint32_t _timer_tval  = 0;
static uint32_t _cntv_us_div;

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
#if __arm__
  __asm__ volatile ("mrc p15, 0, %0, c14, c0, 0" : "=r"(val) );
#elif __aarch64__
  __asm__ volatile("mrs %0, CNTFRQ_EL0" : "=r" (val) : : "memory");
#endif
  return val;
}

static inline uint64_t read_cntvct(void)
{
#if __arm__
	uint32_t low;
	uint32_t high;
   __asm__ volatile ("mrc p15, 0, %0, c14, c3, 0" : "=r"(low) );
   __asm__ volatile ("mrc p15, 0, %0, c14, c3, 1" : "=r"(high) );
   return high << 32 + low;
#else
    uint64_t val;
    __asm__ volatile("mrs %0, CNTVCT_EL0\n\t" : "=r" (val) : : "memory");
    return val;
#endif
}

static inline void enable_cntv(void) {
#if __arm__
	__asm__ volatile ("mcr p15, 0, %0, c14, c3, 1" :: "r"(1));
#elif __aarch64__
	__asm__ volatile("msr CNTV_CTL_EL0, %0":: "r"(1):  "memory");
#endif
}

void timer_init_pix(void){
	 enable_cntv();
	_cntv_us_div = read_cntfrq()/1000000;
}

void timer_set_interval_pix(uint32_t id, uint32_t times_per_sec) {
	(void)id;
	_timer_tval = read_cntfrq() / times_per_sec;
	enable_cntv();
	write_cntv_tval(_timer_tval);
}

void timer_clear_interrupt_pix(uint32_t id) {
	(void)id;
	write_cntv_tval(_timer_tval);
}

uint64_t timer_read_sys_usec_pix(void) { //read microsec
#if __arm__
	#define SYSTEM_TIMER_BASE (_sys_info.mmio.v_base+0x3000)
	#define SYSTEM_TIMER_LOW  0x0004 // System Timer Counter Upper 32 bits
	#define SYSTEM_TIMER_HI   0x0008 // System Timer Counter Upper 32 bits
	uint64_t r = get32(SYSTEM_TIMER_BASE + SYSTEM_TIMER_HI);
	r <<= 32;
	return (r + get32(SYSTEM_TIMER_BASE + SYSTEM_TIMER_LOW));
#elif __aarch64__
	return read_cntvct()/_cntv_us_div;
#endif
}
