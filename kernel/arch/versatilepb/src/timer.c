#include <timer.h>
#include <types.h>
#include <mm/mmu.h>
#include <hardware.h>

/*
The ARM Versatile 926EJS board contains two ARM SB804 dual timer modules [ARM Timers 2004]. Each timer module contains two timers, which are driven by the same clock. The base addresses of the timers are:
	Timer0: 0x101E2000, Timer1: 0x101E2020
	Timer2: 0x101E3000, Timer3: 0x101E3020
*/
#define TIMER ((volatile uint32_t*)(MMIO_BASE+0x001e2000))

#define TIMER_LOAD    0x00
#define TIMER_VALUE   0x01
#define TIMER_CONTROL 0x02
#define TIMER_INTCTL  0x03
#define TIMER_BGLOAD  0x06

void timer_set_interval(uint32_t intervalMicrosecond) {
	TIMER[TIMER_CONTROL] = 0;
	TIMER[TIMER_BGLOAD] = 0;
	TIMER[TIMER_LOAD] = intervalMicrosecond;
	TIMER[TIMER_CONTROL] = 0xe2;	
}

void timer_clear_interrupt(void)
{
	TIMER[TIMER_INTCTL] = 0;
}

void timer_init() {
}

