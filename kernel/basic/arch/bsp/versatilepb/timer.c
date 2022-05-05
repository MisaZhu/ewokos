#include <mm/mmu.h>
#include <dev/mmio.h>
#include <dev/timer.h>
#include <kernel/irq.h>
#include <basic_math.h>

/*
The ARM Versatile 926EJS board contains two ARM SB804 dual timer modules [ARM Timers 2004]. Each timer module contains two timers, which are driven by the same clock. The base addresses of the timers are:
  Timer0: 0x101E2000, Timer1: 0x101E2020
  Timer2: 0x101E3000, Timer3: 0x101E3020
*/
#define TIMER0 ((volatile uint32_t*)(_mmio_base+0x001e2000))
#define TIMER1 ((volatile uint32_t*)(_mmio_base+0x001e2020))
#define TIMER2 ((volatile uint32_t*)(_mmio_base+0x001e3000))
#define TIMER3 ((volatile uint32_t*)(_mmio_base+0x001e3020))

#define TIMER_LOAD    0x00
#define TIMER_VALUE   0x01
#define TIMER_CTRL 0x02
#define TIMER_INTCTRL  0x03
#define TIMER_BGLOAD  0x06

#define TIMER_CTRL_EN	(1 << 7)
#define TIMER_CTRL_FREERUN	(0 << 6)
#define TIMER_CTRL_PERIODIC	(1 << 6)
#define TIMER_CTRL_INTREN	(1 << 5)
#define TIMER_CTRL_DIV1	(0 << 2)
#define TIMER_CTRL_DIV16	(1 << 2)
#define TIMER_CTRL_DIV256	(2 << 2)
#define TIMER_CTRL_32BIT	(1 << 1)
#define TIMER_CTRL_ONESHOT	(1 << 0)

#define	DEFAULT_FREQUENCY	1000000
/* QEMU seems to have problem with full frequency */
#define	DEFAULT_DIVISOR		16
#define	DEFAULT_CTRL_DIV	TIMER_CTRL_DIV16

static volatile uint32_t* timer_addr_by_id(uint32_t id) {
	switch(id) {
		case 0:
			return TIMER0;
		case 1:
			return TIMER1;
		case 2:
			return TIMER2;
		case 3:
			return TIMER3;
	}
	return TIMER0;
}

static uint32_t _sys_usec_tic = 0;
static uint32_t _times_per_sec = 256;

void timer_set_interval(uint32_t id, uint32_t times_per_sec) {
	_sys_usec_tic = 0;
	_times_per_sec = 512;
	volatile uint32_t* t = timer_addr_by_id(id);
	if(times_per_sec > 0)
		_times_per_sec = times_per_sec/4;
	put32(t + TIMER_LOAD, _times_per_sec);
	uint8_t reg = TIMER_CTRL_32BIT | TIMER_CTRL_INTREN |
		TIMER_CTRL_PERIODIC | DEFAULT_CTRL_DIV | TIMER_CTRL_EN;
	put8(t + TIMER_CTRL, reg);
}

void timer_clear_interrupt(uint32_t id) {
	volatile uint32_t* t = timer_addr_by_id(id);
	put32(t + TIMER_INTCTRL, 0xFFFFFFFF);
}

uint64_t timer_read_sys_usec(void) { //read microsec
	_sys_usec_tic += div_u32(DEFAULT_FREQUENCY, _times_per_sec*4);
	return _sys_usec_tic;
}
