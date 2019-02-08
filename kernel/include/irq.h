#ifndef IRQ_H
#define IRQ_H

#include <types.h>

/* number of interrupt lines */
#define IRQ_COUNT 32

typedef void (*InterruptHandlerT)(void);

extern void registerInterruptHandler(uint32_t line, InterruptHandlerT handler);
extern void handleIRQ(void);

/* architecture specific functions */
extern void enableIRQ(uint32_t line);
extern void getPendingIRQs(bool *result);
extern void irqInit();

#endif
