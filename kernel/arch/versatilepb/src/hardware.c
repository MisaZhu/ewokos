#include <hardware.h>
#include <mm/mmu.h>

unsigned int getPhyRamSize() {
	return 256*MB;
}

unsigned int getMMIOBasePhy() {
	return 0x10000000;
}

unsigned int getMMIOMemSize() {
	return 2*MB;
}

unsigned int getUartIrq() {
	return 12;
}

unsigned int getTimerIrq() {
	return 5;
}

void archSetKernelVM(PageDirEntryT* vm) {
	(void)vm;
	uint32_t fbBase = 128*MB; //framebuffer addr
	mapPages(vm, fbBase, fbBase, fbBase+2*MB, AP_RW_D);
}
