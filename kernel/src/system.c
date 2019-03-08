#include <system.h>
#include <types.h>

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

