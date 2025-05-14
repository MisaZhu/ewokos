#include <stdint.h>
#include <gic.h>

#define mmio_read8(addr)         (*((volatile uint8_t  *)(addr)))
#define mmio_read32(addr)         (*((volatile uint32_t *)(addr)))
#define mmio_write8(addr, v)     (*((volatile uint8_t  *)(addr)) = (unsigned char)(v))
#define mmio_write32(addr, v)     (*((volatile uint32_t  *)(addr)) = (unsigned long)(v))

#define NUM_INTSRC    128 // numbers of interrupt source supported

// for yield calling on interrupt
extern struct proc *proc;

// global variables
static uint8_t *gicc_base, *gicd_base;

// not used for now
uint8_t* get_gic_baseaddr(void)
{
    return 0;
}

void gic_init(uint8_t* gicbase) {

    if(!gicbase) 
        gicbase = get_gic_baseaddr();

    gicd_base = gicbase + GICD_OFFSET; // global variable
    gicc_base = gicbase + GICC_OFFSET; // global variable

    // Global enable forwarding interrupts from distributor to cpu interface
    mmio_write32(gicd_base + GICD_CTLR, GICD_CTLR_GRPEN1);
    
    // Global enable signalling of interrupt from the cpu interface
    mmio_write32(gicc_base + GICC_CTLR, GICC_CTLR_GRPEN1);
    mmio_write32(gicc_base + GICC_PMR, GICC_PMR_DEFAULT);
}


uint8_t *gic_v2_gicd_get_address(void)
{
    return gicd_base;
}

uint8_t *gic_v2_gicc_get_address(void)
{
    return gicc_base;
}

void gic_v2_irq_enable(unsigned int irqn)
{
    uint32_t  m ,n, val;
    void *address;
    m = irqn;
    n = m / 32;
    address = gicd_base + GICD_ISENABLER + 4*n;

    val = 1 << (m % 32);
    mmio_write32(address, val);
}

void gic_v2_irq_disable(unsigned int irqn)
{
    uint32_t m ,n, val;
    void *address;
    m = irqn;
    n = m / 32;
    address = gicd_base + GICD_ICENABLE + 4*n;

    val = 1 << (m % 32);
    mmio_write32(address, val);
}


void gic_v2_irq_set_prio(int irqno, int prio)
{
    /* See doc: ARM Generic Interrupt Controller: Architecture Specification version 2.0
     * section 4.3.11
     */
    int n, m, offset;
    volatile uint8_t *gicd = gic_v2_gicd_get_address();
    m = irqno;
    n = m / 4;
    offset = GICD_IPRIORITYR + 4*n;
    offset += m % 4; /* Byte offset */
    
    gicd[offset] = 0xff;
    
    gicd[offset] = prio << 4;  
}

void gic_irq_enable(int core_id, int irqno)
{
    volatile uint8_t *gicd = gic_v2_gicd_get_address() + GICD_ITARGETSR;
    volatile uint8_t mask = 0x1 << core_id;
    int n, m, offset;
    m = irqno;
    n = m / 4;
    offset = 4*n;
    offset += m % 4;

    mask |= mmio_read8(gicd + offset);
    mmio_write8(gicd + offset,  mask);
    gic_v2_irq_set_prio(irqno, LOWEST_INTERRUPT_PRIORITY);  
    gic_v2_irq_enable(irqno);  
}

void gic_irq_disable(int core_id, int irqno)
{
    volatile uint8_t *gicd = gic_v2_gicd_get_address() + GICD_ITARGETSR;
    volatile uint8_t mask = 0x1 << core_id;
    int n, m, offset;
    m = irqno;
    n = m / 4;
    offset = 4*n;
    offset += m % 4;

    mask |= mmio_read8(gicd + offset);
    mmio_write8(gicd + offset,  mask);
    gic_v2_irq_disable(irqno);  
}


void gic_eoi(int intn) {
    mmio_write32(gicc_base + GICC_EOIR, intn); 
}

int gic_get_irq(void){
    int irq = mmio_read32(gicc_base + GICC_IAR);
    gic_eoi(irq);
    return irq;
}

void gic_gen_soft_irq(int core_id, int irq){
    //unsigned long flags;
    uint8_t cpu_mask = 0x1 << core_id;

    /* this always happens on GIC0 */
    mmio_write32(gicd_base + GICD_SGIR, cpu_mask << 16 | irq);
}

int gic_getack( void ) {
    return mmio_read32(gicc_base + GICC_IAR);
}

