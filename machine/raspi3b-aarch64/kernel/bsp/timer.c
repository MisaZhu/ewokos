#include <mm/mmu.h>
#include <kernel/hw_info.h>
#include <kernel/kernel.h>
#include <dev/timer.h>
#include <kernel/irq.h>
#include <kprintf.h>

static uint32_t raw_read_cntv_ctl(void)
{
	uint32_t cntv_ctl;
	__asm__ __volatile__("mrs %0, CNTV_CTL_EL0\n\t" : "=r" (cntv_ctl) :  : "memory");
	return cntv_ctl;
}

static void disable_cntv(void)
{
	uint32_t cntv_ctl;
	cntv_ctl = raw_read_cntv_ctl();
	cntv_ctl &= ~0x1;
	__asm__ __volatile__("msr CNTV_CTL_EL0, %0\n\t" : : "r" (cntv_ctl) : "memory");
}

void enable_cntv(void)
{
	uint32_t cntv_ctl;

	cntv_ctl = raw_read_cntv_ctl();
	cntv_ctl |= 0x1;
	__asm__ __volatile__("msr CNTV_CTL_EL0, %0\n\t" : : "r" (cntv_ctl) : "memory");
}

static uint32_t raw_read_cntfrq_el0(void)
{
	uint32_t cntfrq_el0;

	__asm__ __volatile__("mrs %0, CNTFRQ_EL0\n\t" : "=r" (cntfrq_el0) : : "memory");
	return cntfrq_el0;
}

static void raw_write_cntfrq_el0(uint32_t cntfrq_el0)
{
	__asm__ __volatile__("msr CNTFRQ_EL0, %0\n\t" : : "r" (cntfrq_el0) : "memory");
}

static uint64_t raw_read_cntvct_el0(void)
{
	uint64_t cntvct_el0;

	__asm__ __volatile__("mrs %0, CNTVCT_EL0\n\t" : "=r" (cntvct_el0) : : "memory");
	return cntvct_el0;
}

static uint64_t raw_read_cntv_cval_el0(void)
{
	uint64_t cntv_cval_el0;

	__asm__ __volatile__("mrs %0, CNTV_CVAL_EL0\n\t" : "=r" (cntv_cval_el0) : : "memory");
	return cntv_cval_el0;
}

static void raw_write_cntv_cval_el0(uint64_t cntv_cval_el0)
{
	__asm__ __volatile__("msr CNTV_CVAL_EL0, %0\n\t" : : "r" (cntv_cval_el0) : "memory");
}

static void raw_write_cntv_tval_el0(uint64_t cntv_cval_el0)
{
	__asm__ __volatile__("msr CNTV_TVAL_EL0, %0\n\t" : : "r" (cntv_cval_el0) : "memory");
}


static uint32_t _cntv_step;
static uint32_t _cntv_us_div;

void timer_init(void){
	disable_cntv();
	_cntv_us_div = raw_read_cntfrq_el0()/1000000;
	enable_cntv();
}

void timer_clear_interrupt(uint32_t id) {
	(void)id;
	raw_write_cntv_tval_el0(_cntv_step);	
}

void timer_set_interval(uint32_t id, uint32_t times_per_sec){
	(void)id;
	timer_init();
	_cntv_step = raw_read_cntfrq_el0() / times_per_sec;
	timer_clear_interrupt(id);
}

uint64_t timer_read_sys_usec(void) { //read usec
    return raw_read_cntvct_el0()/_cntv_us_div;
}
