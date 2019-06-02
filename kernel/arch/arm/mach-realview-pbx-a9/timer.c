#include <timer.h>
#include <types.h>
#include <mm/mmu.h>
#include <hardware.h>

#define TIMER ((volatile uint32_t*)(MMIO_BASE+0x0011000))

#define TIMER_LOAD    0x00
#define TIMER_VALUE   0x01
#define TIMER_CONTROL 0x02
#define TIMER_INTCTL  0x03
#define TIMER_BGLOAD  0x06

void timer_set_interval(uint32_t intervalMicrosecond) {
	put8(TIMER + TIMER_CONTROL, 0);
	put8(TIMER + TIMER_BGLOAD, 0);
	put8(TIMER + TIMER_LOAD, intervalMicrosecond);
	put8(TIMER + TIMER_CONTROL, 0xe2);	
}

void timer_clear_interrupt(void) {
	put32(TIMER + TIMER_INTCTL, 0xFFFFFFFF);
}

void timer_init() {
}
