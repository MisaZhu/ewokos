#include <system.h>
#include <types.h>
#include <mm/kalloc.h>
#include <hardware.h>

#define CPSR_IRQ_INHIBIT (1<<7)
#define CPSR_FIQ_INHIBIT (1<<6)
#define NO_INT  (CPSR_IRQ_INHIBIT | CPSR_FIQ_INHIBIT)
#define DIS_INT CPSR_IRQ_INHIBIT

// disable interrupt
void cli(void) {
	uint32_t val;

	__asm__("mrs %[v], cpsr":[v]"=r"(val)::);
	val |= DIS_INT;
	__asm__("msr cpsr_c, %[v]"::[v]"r"(val):);
}

// enable interrupt
void sti(void) {
	uint32_t val;

	__asm__("mrs %[v], cpsr":[v]"=r"(val)::);
	val &= ~DIS_INT;
	__asm__("msr cpsr_c, %[v]"::[v]"r"(val):);
}

inline void loopd(uint32_t times) {
	while(times > 0)
		times--;
}

int32_t system_cmd(int32_t cmd, int32_t arg) {
	(void)arg;
	int32_t ret = -1;
	switch(cmd) {
	case 0: //get total mem size
		ret = (int32_t)get_phy_ram_size();
		break;
	case 1: //get free mem size
		ret = (int32_t)get_free_mem_size();
		break;
	default:
		break;
	}
	return ret;
}
