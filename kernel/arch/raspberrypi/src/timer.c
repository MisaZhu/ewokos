#include <timer.h>
#include <types.h>
#include <mm/mmu.h>
#include <hardware.h>

/* memory mapping for timer */
#define TIMER MMIO_P2V(0x20003000)

#define CS 0x20003000
#define CLO 0x20003004
#define C0 0x2000300C
#define C1 0x20003010
#define C2 0x20003014
#define C3 0x20003018


static void PUT32(uint32_t address, uint32_t value) {
	*MMIO_P2V(address) = value;
}

static uint32_t GET32(uint32_t address) {
	return *MMIO_P2V(address);
}

static uint32_t savedIntervalMicrosecond;


void timerSetInterval(uint32_t intervalMicrosecond)
{
	savedIntervalMicrosecond = intervalMicrosecond;
	timerClearInterrupt();
}

void timerClearInterrupt(void)
{
	unsigned int ra;

	PUT32(CS, 2);

	ra = GET32(CLO);
	ra += savedIntervalMicrosecond;
	PUT32(C1, ra);
}

