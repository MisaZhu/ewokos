#ifndef HARDWARE_H
#define HARDWARE_H

extern unsigned int getMMIOBasePhy();
extern unsigned int getMMIOMemSize();

extern unsigned int getUartIrq();
extern unsigned int getTimerIrq();

extern unsigned int getPhyRamSize();

#endif
