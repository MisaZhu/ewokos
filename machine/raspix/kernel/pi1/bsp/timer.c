#include <dev/timer.h>
#include <kernel/irq.h>
#include <kernel/hw_info.h>
#include <kernel/kernel.h>

#define SYSTEM_TIMER_BASE (_sys_info.mmio.v_base+0x3000)
#define SYSTEM_TIMER_LOW  0x0004 // System Timer Counter Upper 32 bits
#define SYSTEM_TIMER_HI   0x0008 // System Timer Counter Upper 32 bits

#define ARM_TIMER_LOD (_sys_info.mmio.v_base+0x0B400)
#define ARM_TIMER_VAL (_sys_info.mmio.v_base+0x0B404)
#define ARM_TIMER_CTL (_sys_info.mmio.v_base+0x0B408)
#define ARM_TIMER_CLI (_sys_info.mmio.v_base+0x0B40C)
#define ARM_TIMER_RIS (_sys_info.mmio.v_base+0x0B410)
#define ARM_TIMER_MIS (_sys_info.mmio.v_base+0x0B414)
#define ARM_TIMER_RLD (_sys_info.mmio.v_base+0x0B418)
#define ARM_TIMER_DIV (_sys_info.mmio.v_base+0x0B41C)
#define ARM_TIMER_CNT (_sys_info.mmio.v_base+0x0B420)

void timer_set_interval(uint32_t id, uint32_t times_per_sec) {
	(void)id;

	put32(ARM_TIMER_CTL, 0x003E0000);
	put32(ARM_TIMER_LOD, times_per_sec * 10 - 1);
	put32(ARM_TIMER_RLD, times_per_sec * 10 - 1);
	put32(ARM_TIMER_CLI, 0);
	put32(ARM_TIMER_CTL, 0x003E00A2);
	put32(ARM_TIMER_CLI, 0);
}

void timer_clear_interrupt(uint32_t id) {
	(void)id;
	put32(ARM_TIMER_CLI,0);
}

uint64_t timer_read_sys_usec(void) { //read microsec
	uint64_t r = get32(SYSTEM_TIMER_BASE + SYSTEM_TIMER_HI);
	r <<= 32;
	return (r + get32(SYSTEM_TIMER_BASE + SYSTEM_TIMER_LOW));
}
