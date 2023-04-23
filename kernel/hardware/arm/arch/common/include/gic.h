#ifndef _GIC_V2_H_
#define _GIC_V2_H_

/*******************************************************************************
 * GIC Distributor interface general definitions
 ******************************************************************************/
/* Constants to categorise interrupts */
#define MIN_SGI_ID		U(0)
#define MIN_SEC_SGI_ID		U(8)
#define MIN_PPI_ID		U(16)
#define MIN_SPI_ID		U(32)
#define MAX_SPI_ID		U(1019)

#define TOTAL_SPI_INTR_NUM	(MAX_SPI_ID - MIN_SPI_ID + U(1))
#define TOTAL_PCPU_INTR_NUM	(MIN_SPI_ID - MIN_SGI_ID)

/* Mask for the priority field common to all GIC interfaces */
#define GIC_PRI_MASK			U(0xff)

/* Mask for the configuration field common to all GIC interfaces */
#define GIC_CFG_MASK			U(0x3)

/* Constant to indicate a spurious interrupt in all GIC versions */
#define GIC_SPURIOUS_INTERRUPT		U(1023)

/* Interrupt configurations: 2-bit fields with LSB reserved */
#define GIC_INTR_CFG_LEVEL		(0 << 1)
#define GIC_INTR_CFG_EDGE		(1 << 1)

/* Highest possible interrupt priorities */
#define GIC_HIGHEST_SEC_PRIORITY	U(0x00)
#define GIC_HIGHEST_NS_PRIORITY		U(0x80)

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
#define GICD_SGIR           0xF00

#define GICC_CTLR       0x0000
#define GICC_CTLR_GRPEN1  (1 << 0)
#define GICC_PMR        0x0004
#define GICC_PMR_DEFAULT  0xf0

#define GICC_BPR    0x008
#define GICC_IAR    0x00C
#define GICC_EOIR   0x010
#define GICC_RRR    0x014
#define GICC_HPPIR   0x018
#define GICC_AEOIR   0x024

#define configUNIQUE_INTERRUPT_PRIORITIES        16
#define portLOWEST_INTERRUPT_PRIORITY ( ( ( uint32_t ) configUNIQUE_INTERRUPT_PRIORITIES ) - 1UL )
#define portLOWEST_USABLE_INTERRUPT_PRIORITY ( portLOWEST_INTERRUPT_PRIORITY - 1UL )
#define LOWEST_INTERRUPT_PRIORITY 0xE

/*******************************************************************************
 * Common GIC Distributor interface register constants
 ******************************************************************************/
#define PIDR2_ARCH_REV_SHIFT	4
#define PIDR2_ARCH_REV_MASK	U(0xf)

/* GIC revision as reported by PIDR2.ArchRev register field */
#define ARCH_REV_GICV1		U(0x1)
#define ARCH_REV_GICV2		U(0x2)
#define ARCH_REV_GICV3		U(0x3)
#define ARCH_REV_GICV4		U(0x4)

#define IGROUPR_SHIFT		5
#define ISENABLER_SHIFT		5
#define ICENABLER_SHIFT		ISENABLER_SHIFT
#define ISPENDR_SHIFT		5
#define ICPENDR_SHIFT		ISPENDR_SHIFT
#define ISACTIVER_SHIFT		5
#define ICACTIVER_SHIFT		ISACTIVER_SHIFT
#define IPRIORITYR_SHIFT	2
#define ITARGETSR_SHIFT		2
#define ICFGR_SHIFT		4
#define NSACR_SHIFT		4

/* GICD_TYPER shifts and masks */
#define TYPER_IT_LINES_NO_SHIFT	U(0)
#define TYPER_IT_LINES_NO_MASK	U(0x1f)

/* Value used to initialize Normal world interrupt priorities four at a time */
#define GICD_IPRIORITYR_DEF_VAL			\
	(GIC_HIGHEST_NS_PRIORITY	|	\
	(GIC_HIGHEST_NS_PRIORITY << 8)	|	\
	(GIC_HIGHEST_NS_PRIORITY << 16)	|	\
	(GIC_HIGHEST_NS_PRIORITY << 24))

void gic_init(uint8_t *gicbase);
void gic_irq_enable(int core_id, int irqno);
int gic_get_irq(void);
void gic_gen_soft_irq(int core, int irq);
#endif
