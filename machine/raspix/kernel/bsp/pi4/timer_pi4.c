#include <dev/timer.h>
#include <kernel/irq.h>
#include <kernel/kernel.h>
#include <mm/mmu.h>
#include "../timer_arch.h"

#define GIC_DEFAULT_FREQ	6000000
uint32_t _timer_tval  = 0;
uint32_t _cntfrq = GIC_DEFAULT_FREQ;

void __write_cntv_tval(uint32_t);
void __enable_cntv(void);

static inline uint32_t read_cntfrq(void) {
#if __arm__
  uint32_t val;
  __asm__ volatile ("mrc p15, 0, %0, c14, c0, 0" : "=r"(val) );
#elif __aarch64__
  uint64_t val;
  __asm__ volatile("mrs %0, CNTFRQ_EL0" : "=r" (val) : : "memory");
#endif
  return val;
}

static inline uint32_t read_cntpct(void) {
  uint32_t val;
#if __arm__
  __asm__ volatile ("mrc p15, 0, %0, c14, c0, 0" : "=r"(val) );
#elif __aarch64__
   __asm__ volatile("mrs %0, CNTPCT_EL0" : "=r" (val) : : "memory");
#endif
  return val;
}

inline void write_cntv_tval(uint32_t tval) {
#if __arm__
	__asm__ volatile ("mcr p15, 0, %0, c14, c3, 0" :: "r"(tval));
#elif __aarch64__
   __asm__ volatile("msr CNTV_TVAL_EL0, %0" :: "r" (tval) : "memory");
#endif

}

static inline uint32_t read_cntv_tval(void) {
	uint32_t val;
#if __arm__
	__asm__ volatile ("mrc p15, 0, %0, c14, c3, 0" :: "r"(val));
#elif __aarch64__
   __asm__ volatile("mrs %0, CNTV_TVAL_EL0" : "=r" (val) : : "memory");
#endif

	return val;
}


static inline void enable_cntv(void) {
#if __arm__
	__asm__ volatile ("mcr p15, 0, %0, c14, c3, 1" :: "r"(1));
#elif __aarch64__
     uint64_t ctl;
     __asm__ volatile("mrs %0, CNTV_CTL_EL0" : "=r" (ctl) : : "memory");
     ctl |= 0x1;
     __asm__ volatile("msr CNTV_CTL_EL0, %0":: "r"(ctl):  "memory");
#endif

}

static inline uint32_t disable_cntv(void) {
#if __arm__
	__asm__ volatile("mcr p15, 0, %0, c14, C3, 1" :: "r" (0));
#elif __aarch64__
  uint32_t ctl;
     __asm__ volatile("mrs %0, CNTV_CTL_EL0" : "=r" (ctl) : : "memory");
     ctl &= ~0x1;
     __asm__ volatile("msr CNTV_CTL_EL0, %0":: "r"(ctl):  "memory");
#endif

}

static inline uint64_t  read_cntvct(void) {
	uint64_t val;
#if __arm__
	__asm__ volatile("mrrc p15, 1, %Q0, %R0, c14" : "=r" (val));
#elif __aarch64__
   __asm__ volatile("mrs %0, CNTVCT_EL0" : "=r" (val) : : "memory");
#endif
	return val;
}

static inline uint32_t read_cntctl(void) {
	uint32_t val;
#if __arm__
	__asm__ volatile("mrc p15, 0, %0, c14, C3, 1" : "=r" (val));
#elif __aarch64__
   __asm__ volatile("mrs %0, CNTCTL_EL0" : "=r" (val) : : "memory");
#endif
	return val;
}

static uint32_t _cntv_step;
static uint32_t _cntv_us_div;

void timer_init(void){
    disable_cntv();
    _cntv_us_div = read_cntfrq()/1000000;
    enable_cntv();
}

inline void timer_clear_interrupt(uint32_t id) {
	(void)id;
	write_cntv_tval(_timer_tval);
}

void timer_set_interval(uint32_t id, uint32_t times_per_sec) {
	(void)id;
    timer_init();
    _cntv_step = read_cntfrq() / times_per_sec;
    timer_clear_interrupt(id);
}

inline uint64_t timer_read_sys_usec(void) { //read microsec
	return read_cntvct()/_cntv_us_div;
}
