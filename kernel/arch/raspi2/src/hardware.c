#include <hardware.h>
#include <mm/mmu.h>
#include <mailbox.h>


uint32_t getPhyRamSize() {
	TagsInfoT info;
	if(mailboxGetBoardInfo(&info) == NULL)
		return 256*MB;
	return info.memory_arm_size+info.memory_vc_size;
}

uint32_t getMMIOBasePhy() {
	return 0x3F000000;
}

uint32_t getMMIOMemSize() {
	return 32*MB;
}

uint32_t getUartIrq() {
	return 25;
}

uint32_t getTimerIrq() {
	return 0;
}

