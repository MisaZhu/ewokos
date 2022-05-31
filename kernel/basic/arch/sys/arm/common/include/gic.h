#ifndef _GIC_V2_H_
#define _GIC_V2_H_

// only for reference
//#define GICD_V2_BASE  ((void *)0x01c81000)
//#define GICC_V2_BASE  ((void *)0x01c82000)

#define GICD_OFFSET 0x1000
#define GICC_OFFSET 0x2000
#define GICH_OFFSET 0x4000
#define GICV_OFFSET 0x6000

#define GICD_CTLR    0x0000
#define GICD_CTLR_GRPEN1  (1 << 0)

#define GICD_TYPER   0x004
#define GICD_IIDR    0x008
#define GICD_IGROUP  0x080

#define GICD_ISENABLER		0x0100
#define GICD_ICENABLE		0x180
#define GICD_ISPEND			0x200
#define GICD_ICPEND			0x280
#define GICD_ISACTIVE		0x300
#define GICD_ICACTIVE		0x380
#define GICD_IPRIORITYR     0x0400
#define GICD_ITARGETSR      0x0800
#define GICD_ICFG			0xC00

#define GICC_CTLR       0x0000
#define GICC_CTLR_GRPEN1  (1 << 0)
#define GICC_PMR        0x0004
#define GICC_PMR_DEFAULT  0xf0

#define GICC_BPR    0x008
#define GICC_IAR    0x00C
#define GICC_EOIR   0x010
#define GICC_RRR    0x014
#define GICC_HPPIR   0x018

#define configUNIQUE_INTERRUPT_PRIORITIES        16
#define portLOWEST_INTERRUPT_PRIORITY ( ( ( uint32_t ) configUNIQUE_INTERRUPT_PRIORITIES ) - 1UL )
#define portLOWEST_USABLE_INTERRUPT_PRIORITY ( portLOWEST_INTERRUPT_PRIORITY - 1UL )
#define LOWEST_INTERRUPT_PRIORITY 0xE


void gic_init(void);
void gic_irq_enable(int irqno);
uint32_t gic_get_irq(int irqn);
#endif
