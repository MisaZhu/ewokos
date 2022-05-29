#include <mm/mmu.h>
#include <dev/timer.h>
#include <kernel/irq.h>
#include <basic_math.h>
#include "timer_arch.h"

uint32_t _timer_tval  = 0;

uint32_t _cntfrq = 5000000;

void __write_cntv_tval(uint32_t);
void __enable_cntv(void);

static inline uint32_t read_cntfrq(void) {
  uint32_t val;
  __asm__ volatile ("mrc p15, 0, %0, c14, c0, 0" : "=r"(val) );
  return val;
}

static inline uint32_t read_cntpct(void) {
  uint32_t val;
  __asm__ volatile ("mrc p15, 0, %0, c14, c0, 0" : "=r"(val) );
  return val;
}

inline void write_cntv_tval(uint32_t tval) {
	__asm__ volatile ("mcr p15, 0, %0, c14, c3, 0" :: "r"(tval));
}

static inline uint32_t read_cntv_tval(void) {
	uint32_t val;
	__asm__ volatile ("mrc p15, 0, %0, c14, c3, 0" :: "r"(val));
	return val;
}


static inline void enable_cntv(void) {
	__asm__ volatile ("mcr p15, 0, %0, c14, c3, 1" :: "r"(1));
}

static inline uint32_t read_cntvct(void) {
	uint32_t val;
	__asm__ volatile("mrrc p15, 1, %Q0, %R0, c14" : "=r" (val));
}

static inline uint32_t read_cntctl(void) {
	uint32_t val;
	__asm__ volatile("mrc p15, 0, %0, c14, C3, 1" : "=r" (val));
}

#define MIN_FREQ 4096 
void timer_set_interval(uint32_t id, uint32_t times_per_sec) {
	(void)id;
	if (times_per_sec < MIN_FREQ)
		times_per_sec = MIN_FREQ;
	printf("counter freq: %d\n", read_cntfrq());
	//_timer_tval = read_cntfrq() / times_per_sec /20;
	_timer_tval = _cntfrq/times_per_sec/20;
	write_cntv_tval(_timer_tval);
	enable_cntv();
}

void timer_clear_interrupt(uint32_t id) {
	write_cntv_tval(_timer_tval);
	(void)id;
}



uint64_t timer_read_sys_usec(void) { //read microsec
	return read_cntvct() >>  2;///(_cntfrq/100000);
	//return read_cntctl();
	//return read_cntv_tval();
}
