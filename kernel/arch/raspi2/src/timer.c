#include <timer.h>
#include <types.h>
#include <mm/mmu.h>
#include <hardware.h>

/* memory mapping for timer */
#define TIMER_OFF 0x3000

#define CS  TIMER_OFF

#define CLO 0x3004
#define C0  0x300C
#define C1  0x3010
#define C2  0x3014
#define C3  0x3018

static uint32_t savedIntervalMicrosecond;

void timerSetInterval(uint32_t intervalMicrosecond) {
	savedIntervalMicrosecond = intervalMicrosecond;
	timerClearInterrupt();
}

void timerClearInterrupt(void) {
	unsigned int ra;

	mmio_write(MMIO_BASE+CS, 2);

	ra = mmio_read(MMIO_BASE+CLO);
	ra += savedIntervalMicrosecond;
	mmio_write(MMIO_BASE+C1, ra);
}


#define PIC_BASE 0xB200
#define PIC_ENABLE_BASIC_IRQ PIC_BASE+(4*6)

#define ARM_TIMER_BASE 0xB400
#define ARM_TIMER_LOAD ARM_TIMER_BASE
#define ARM_TIMER_CTRL ARM_TIMER_BASE+(4*2)
#define ARM_TIMER_CTRL_32BIT (1<<1)
#define ARM_TIMER_CTRL_ENABLE (1<<7)
#define ARM_TIMER_CTRL_IRQ_ENABLE (1<<5)
#define ARM_TIMER_CTRL_PRESCALE_256 (2<<2)

#define IRQ_ARM_TIMER_BIT 0
#define IRQ_SYS_TIMER_BIT 1

void timerInit() {
	mmio_write(MMIO_BASE + PIC_ENABLE_BASIC_IRQ, 1 << IRQ_ARM_TIMER_BIT);
	mmio_write(MMIO_BASE + PIC_ENABLE_BASIC_IRQ, 1 << IRQ_SYS_TIMER_BIT);
/*	mmio_write(ARM_TIMER_LOAD, 0x400);
	mmio_write(ARM_TIMER_CTRL, ARM_TIMER_CTRL_32BIT | ARM_TIMER_CTRL_ENABLE |
			ARM_TIMER_CTRL_IRQ_ENABLE | ARM_TIMER_CTRL_PRESCALE_256);
			*/
}
