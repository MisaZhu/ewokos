#ifndef HARDWARE_H
#define HARDWARE_H

#define MMIO_BASE_PHY 0x10000000
#define MMIO_MEM_SIZE (4*MB)

#define UART_IRQ  12
#define TIMER_IRQ 5

extern unsigned int getPhyRamSize();

#endif
