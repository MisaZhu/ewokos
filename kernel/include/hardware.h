#ifndef HARDWARE_H
#define HARDWARE_H

#include <types.h>
#include <mm/mmu.h>

extern void hardwareInit();

extern uint32_t getUartIrq();
extern uint32_t getTimerIrq();

extern uint32_t getMMIOBasePhy();
extern uint32_t getMMIOMemSize();

extern uint32_t getPhyRamSize();

extern void archSetKernelVM(PageDirEntryT* vm);

#endif
