#include <irq.h>
#include <hardware.h>
#include <mmu.h>
#include <types.h>
#include <lib/string.h>

/* memory mapping for the interrupt controller */
#define PIC MMIO_P2V(0x2000B000)

/* interrupt controller register offsets */
#define PIC_STATUS     0x81
#define PIC_INT_ENABLE 0x84

void enableIRQ(int line)
{
	PIC[PIC_INT_ENABLE] = (1 << line);
}

void getPendingIRQs(bool *result)
{
	unsigned int irqStatus = PIC[PIC_STATUS];

	memset(result, 0, IRQ_COUNT * sizeof(bool));
	for (int i = 0; i < 32; i++)
		if (irqStatus & (1u << i))
			result[i] = true;
}

