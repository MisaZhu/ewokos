#include <irq.h>
#include <hardware.h>
#include <mm/mmu.h>
#include <types.h>
#include <string.h>

/* memory mapping for the interrupt controller */
#define PIC ((volatile uint32_t*)(MMIO_BASE+0x00140000))
/* interrupt controller register offsets */
#define PIC_STATUS     0x0
#define PIC_INT_ENABLE 0x4

void enableIRQ(uint32_t line)
{
	PIC[PIC_INT_ENABLE] = (1 << line);
}

void getPendingIRQs(bool *result)
{
	uint32_t irqStatus = PIC[PIC_STATUS];

	memset(result, 0, IRQ_COUNT * sizeof(bool));
	for (uint32_t i = 0; i < 32; i++)
		if (irqStatus & (1u << i))
			result[i] = true;
}

