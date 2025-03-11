#include <kernel/irq.h>
#include <kernel/hw_info.h>
#include <mm/mmu.h>
#include "arch.h"

#define INTC_BASE			(0xFFFEE000)

#define CP_INTC_REV         0x00
#define CP_INTC_CTRL            0x04
#define CP_INTC_HOST_CTRL       0x0C
#define CP_INTC_GLOBAL_ENABLE       0x10
#define CP_INTC_GLOBAL_NESTING_LEVEL    0x1C
#define CP_INTC_SYS_STAT_IDX_SET    0x20
#define CP_INTC_SYS_STAT_IDX_CLR    0x24
#define CP_INTC_SYS_ENABLE_IDX_SET  0x28
#define CP_INTC_SYS_ENABLE_IDX_CLR  0x2C
#define CP_INTC_GLOBAL_WAKEUP_ENABLE    0x30
#define CP_INTC_HOST_ENABLE_IDX_SET 0x34
#define CP_INTC_HOST_ENABLE_IDX_CLR 0x38
#define CP_INTC_PACING_PRESCALE     0x40
#define CP_INTC_VECTOR_BASE     0x50
#define CP_INTC_VECTOR_SIZE     0x54
#define CP_INTC_VECTOR_NULL     0x58
#define CP_INTC_PRIO_IDX        0x80
#define CP_INTC_PRIO_VECTOR     0x84
#define CP_INTC_SECURE_ENABLE       0x90
#define CP_INTC_SECURE_PRIO_IDX     0x94
#define CP_INTC_PACING_PARAM(n)     (0x0100 + (n << 4))
#define CP_INTC_PACING_DEC(n)       (0x0104 + (n << 4))
#define CP_INTC_PACING_MAP(n)       (0x0108 + (n << 4))
#define CP_INTC_SYS_RAW_STAT(n)     (0x0200 + (n << 2))
#define CP_INTC_SYS_STAT_CLR(n)     (0x0280 + (n << 2))
#define CP_INTC_SYS_ENABLE_SET(n)   (0x0300 + (n << 2))
#define CP_INTC_SYS_ENABLE_CLR(n)   (0x0380 + (n << 2))
#define CP_INTC_CHAN_MAP(n)     (0x0400 + (n << 2))
#define CP_INTC_HOST_MAP(n)     (0x0800 + (n << 2))
#define CP_INTC_HOST_PRIO_IDX(n)    (0x0900 + (n << 2))
#define CP_INTC_SYS_POLARITY(n)     (0x0D00 + (n << 2))
#define CP_INTC_SYS_TYPE(n)     (0x0D80 + (n << 2))
#define CP_INTC_WAKEUP_ENABLE(n)    (0x0E00 + (n << 2))
#define CP_INTC_DEBUG_SELECT(n)     (0x0F00 + (n << 2))
#define CP_INTC_SYS_SECURE_ENABLE(n)    (0x1000 + (n << 2))
#define CP_INTC_HOST_NESTING_LEVEL(n)   (0x1100 + (n << 2))
#define CP_INTC_HOST_ENABLE(n)      (0x1500 + (n << 2))
#define CP_INTC_HOST_PRIO_VECTOR(n) (0x1600 + (n << 2))
#define CP_INTC_VECTOR_ADDR(n)      (0x2000 + (n << 2))

#define writel(val, reg)    (*(volatile uint32_t*)(reg) = (val))
#define readl(reg)          (*(volatile uint32_t*)(reg))

/* memory mapping for the prime interrupt controller */
#define cp_intc_write(val, reg) writel(val, INTC_BASE + reg)

void irq_arch_init(void) {
    cp_intc_write(0, CP_INTC_GLOBAL_ENABLE);

    /* Disable all host interrupts */
    cp_intc_write(0, CP_INTC_HOST_ENABLE(0));

    /* Disable system interrupts */
    for (int i = 0; i < 4; i++)
        cp_intc_write(~0, CP_INTC_SYS_ENABLE_CLR(i));

    /* Set to normal mode, no nesting, no priority hold */
    cp_intc_write(0, CP_INTC_CTRL);
    cp_intc_write(0, CP_INTC_HOST_CTRL);

    /* Clear system interrupt status */
    for (int i = 0; i < 4; i++)
        cp_intc_write(~0, CP_INTC_SYS_STAT_CLR(i));

    /* Enable nIRQ (what about nFIQ?) */
    cp_intc_write(1, CP_INTC_HOST_ENABLE_IDX_SET);

    /*
     * Default everything to channel 15 if priority not specified.
     * Note that channel 0-1 are mapped to nFIQ and channels 2-31
     * are mapped to nIRQ.
     */
    for (int i = 0; i < 26; i++){
        cp_intc_write(0x0f0f0f0f, CP_INTC_CHAN_MAP(i));
	}

	/* Enable global interrupt */
    cp_intc_write(1, CP_INTC_GLOBAL_ENABLE);
}

void irq_enable(uint32_t irq) {
	if(irq == IRQ_TIMER0){ 
		  cp_intc_write(21, CP_INTC_SYS_ENABLE_IDX_SET);
	}
}

void irq_disable(uint32_t irq) {
	if(irq == IRQ_TIMER0){
		cp_intc_write(1, CP_INTC_HOST_ENABLE_IDX_CLR);
    	cp_intc_write(21, CP_INTC_SYS_ENABLE_IDX_CLR);
    	cp_intc_write(1, CP_INTC_HOST_ENABLE_IDX_SET);
	}
}

inline uint32_t irq_get(void) {
	cp_intc_write(21, CP_INTC_SYS_STAT_IDX_CLR); 
	return IRQ_TIMER0;
}

