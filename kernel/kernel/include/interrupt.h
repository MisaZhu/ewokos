#ifndef INTR_H
#define INTR_H

#define IRQ_NONE   0x00000000
#define IRQ_TIMER0 0x00000001
#define IRQ_SOFT   0x00001000
#define IRQ_IPI    0x00002000
#define IRQ_RAW_P  0x00004000
#define IRQ_RAW_S  0x00008000

#define SYS_INT_MAX 64

#endif
