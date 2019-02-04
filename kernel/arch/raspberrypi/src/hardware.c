#include <hardware.h>
#include <mm/mmu.h>

unsigned int getPhyRamSize() {
	return 255*MB;
}

unsigned int getMMIOBasePhy() {
	return 0x20000000;
}

unsigned int getMMIOMemSize() {
	return 4*MB;
}

unsigned int getUartIrq() {
	return 29;
}

unsigned int getTimerIrq() {
	return 1;
}

