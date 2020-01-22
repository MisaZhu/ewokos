#ifndef IRQ_H
#define IRQ_H

#include <kernel/context.h>

#define IRQ_TIMER0 0x00000001
#define IRQ_TIMER1 0x00000002
#define IRQ_TIMER2 0x00000004
#define IRQ_TIMER3 0x00000008

#define IRQ_UART0  0x00000010
#define IRQ_UART1  0x00000020
#define IRQ_UART2  0x00000040
#define IRQ_UART3  0x00000080

#define IRQ_SIC    0x00000100

#define IRQ_KEY    0x00000200
#define IRQ_MOUSE  0x00000400
#define IRQ_SDC    0x00000800

extern void irq_handler(context_t* ctx);
extern void prefetch_abort_handler(context_t* ctx);
extern void data_abort_handler(context_t* ctx);
extern void irq_init(void);
extern void irq_arch_init(void);

#endif
