#include <timer.h>
#include <scheduler.h>
#include <hardware.h>
#include <irq.h>

#define SCHEDULE_TIME 1000 /*0.001 sec*/
//static uint32_t _cpu_tick = 0;
static uint32_t _cpu_msec = 0;
static uint32_t _cpu_sec = 0;

void timer_handle() {
	/*
	_cpu_tick++;
	if(_cpu_tick >= 10) {
		_cpu_msec++;
	}
	*/
	_cpu_msec++;
	if(_cpu_msec >= 1000) { 
		_cpu_sec++;
		_cpu_msec = 0;
	}

	timer_clear_interrupt();
	schedule();
}

void timer_start(void) {
	timer_set_interval(SCHEDULE_TIME);
}

void cpu_tick(uint32_t* sec, uint32_t* msec) {
	if(sec != NULL)
		*sec = _cpu_sec;
	if(msec != NULL)
		*msec = _cpu_msec;
}
