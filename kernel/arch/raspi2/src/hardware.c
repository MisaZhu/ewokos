#include <hardware.h>
#include <mm/mmu.h>
#include <mailbox.h>

void hardwareInit() {
	mailboxInit();
}

uint32_t getPhyRamSize() {
	TagsInfoT info;
	if(mailboxGetBoardInfo(&info) == NULL)
		return 256*MB;
	return info.memory_arm_size+info.memory_vc_size;
}

uint32_t getMMIOBasePhy() {
	return 0x3F000000;
}

uint32_t getInitRDBasePhy() {
	return 0x08000000;
}

uint32_t getInitRDSize() {
	return 1*MB;
}

uint32_t getMMIOMemSize() {
	return 4*MB;
}

uint32_t getUartIrq() {
	return 25;
}

uint32_t getTimerIrq() {
	return 0;
}

#define CORE0_ROUTING 0x40000000

void archSetKernelVM(PageDirEntryT* vm) {
	uint32_t offset = CORE0_ROUTING - getMMIOBasePhy();
	uint32_t vbase = MMIO_BASE + offset;
	uint32_t pbase = getMMIOBasePhy() +offset;
	mapPages(vm, vbase, pbase, pbase+16*KB, AP_RW_D);
}
