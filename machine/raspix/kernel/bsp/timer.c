#include <dev/timer.h>
#include <kernel/irq.h>
#include <kernel/kernel.h>
#include <mm/mmu.h>
#include "timer_arch.h"
#include "hw_arch.h"

inline void write_cntv_tval(uint32_t tval) {
#if __arm__
	__asm__ volatile ("mcr p15, 0, %0, c14, c3, 0" :: "r"(tval));
#elif __aarch64__
	__asm__ volatile("msr CNTV_TVAL_EL0, %0" : : "r" (tval) : "memory");
#endif
}

void timer_init_pix(void);
void timer_init_pi4(void);
void timer_init(void){
  if(_pi4)
    timer_init_pi4();
  else
    timer_init_pix();
}

void timer_clear_interrupt_pi4(uint32_t);
void timer_clear_interrupt_pix(uint32_t);
inline void timer_clear_interrupt(uint32_t id) {
  if(_pi4)
    timer_clear_interrupt_pi4(id);
  else
    timer_clear_interrupt_pix(id);
}

void timer_set_interval_pix(uint32_t id, uint32_t times_per_sec);
void timer_set_interval_pi4(uint32_t id, uint32_t times_per_sec);
void timer_set_interval(uint32_t id, uint32_t times_per_sec) {
  if(_pi4)
    timer_set_interval_pi4(id, times_per_sec);
  else
    timer_set_interval_pix(id, times_per_sec);
}

uint64_t timer_read_sys_usec_pix(void);
uint64_t timer_read_sys_usec_pi4(void);
inline uint64_t timer_read_sys_usec(void) { //read microsec
  if(_pi4)
    return timer_read_sys_usec_pi4();
  return timer_read_sys_usec_pix();
}
