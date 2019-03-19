#include <hardware.h>
#include <mm/mmu.h>

void hardwareInit() {
}

uint32_t getPhyRamSize() {
	return 256*MB;
}

uint32_t getMMIOBasePhy() {
	return 0x10000000;
}

uint32_t getMMIOMemSize() {
	return 4*MB;
}

uint32_t getInitRDBasePhy() {
	return 0x08000000;
}

uint32_t getInitRDSize() {
	return 1*MB;
}

uint32_t getUartIrq() {
	return 12;
}

uint32_t getTimerIrq() {
	return 5;
}

void archSetKernelVM(PageDirEntryT* vm) {
	(void)vm;
	uint32_t fbBase = 126*MB; //framebuffer addr
	mapPages(vm, fbBase, fbBase, fbBase+2*MB, AP_RW_D);
}
