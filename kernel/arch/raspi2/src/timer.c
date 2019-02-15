#include <timer.h>
#include <types.h>
#include <mm/mmu.h>
#include <hardware.h>
#include <irq.h>

#define ARM_TIMER_OFF 0xB400
#define ARM_TIMER_CTRL_32BIT (1<<1)
#define ARM_TIMER_CTRL_ENABLE (1<<7)
#define ARM_TIMER_CTRL_IRQ_ENABLE (1<<5)
#define ARM_TIMER_CTRL_PRESCALE_256 (2<<2)

#define IRQ_ARM_TIMER_BIT 0

#define TIMER_LOAD    0x00
#define TIMER_CONTROL 0x02
#define TIMER_INTCTL  0x03
#define TIMER_BGLOAD  0x06

void timerSetInterval(uint32_t intervalMicrosecond) {
	/*volatile uint32_t* timer = (volatile uint32_t*)(MMIO_BASE+ARM_TIMER_OFF);
	timer[TIMER_CONTROL] = 0;
	timer[TIMER_BGLOAD] = 0;
	timer[TIMER_LOAD] = intervalMicrosecond;
	timer[TIMER_CONTROL] = 0x003E00A2;
	*/
}

void timerClearInterrupt(void) {
	//volatile uint32_t* timer = (volatile uint32_t*)(MMIO_BASE+ARM_TIMER_OFF);
	//timer[TIMER_INTCTL] = 0;
}


void timerInit() {
	volatile uint32_t* timer = (volatile uint32_t*)(MMIO_BASE+ARM_TIMER_OFF);
	timer[TIMER_LOAD] = 0x1000;
	timer[TIMER_CONTROL] = 0x003E0000;
	/*timer[TIMER_CONTROL] = (ARM_TIMER_CTRL_32BIT | ARM_TIMER_CTRL_ENABLE |
			ARM_TIMER_CTRL_IRQ_ENABLE | ARM_TIMER_CTRL_PRESCALE_256);
			*/
	timer[TIMER_INTCTL] = 0;
	enableIRQ(IRQ_ARM_TIMER_BIT);
}
