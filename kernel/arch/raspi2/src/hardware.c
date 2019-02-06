#include <hardware.h>
#include <mm/mmu.h>

uint32_t getPhyRamSize() {
	return 1*GB;
}

uint32_t getMMIOBasePhy() {
	return 0x3F000000;
}

uint32_t getMMIOMemSize() {
	return 16*MB;
}

uint32_t getUartIrq() {
	return 29;
}

uint32_t getTimerIrq() {
	return 0;
}

