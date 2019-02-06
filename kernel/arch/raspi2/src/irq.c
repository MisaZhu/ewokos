#include <irq.h>
#include <hardware.h>
#include <mm/mmu.h>
#include <string.h>
#include <types.h>

/* interrupt controller register offsets */
//#define PIC_STATUS     0x81
//#define PIC_INT_ENABLE 0x84
#define PIC_STATUS      0
#define PIC_INT_ENABLE  6
#define PIC_INT_DISABLE 9

#define PIC_BASE 0x3F00B200

void enableIRQ(uint32_t line) {
	volatile uint32_t* vpic = (volatile uint32_t*)P2V(PIC_BASE);
  vpic[PIC_INT_ENABLE] = (1<<line);
}

void disableIRQ(uint32_t line) {
	volatile uint32_t* vpic = (volatile uint32_t*)P2V(PIC_BASE);
  vpic[PIC_INT_DISABLE] = (1<<line);
}

void getPendingIRQs(bool *result) {
	volatile uint32_t* vpic = (volatile uint32_t*)P2V(PIC_BASE);
	volatile uint32_t irqStatus = vpic[PIC_STATUS];

	memset(result, 0, IRQ_COUNT * sizeof(bool));
	for (uint32_t i = 0; i < 32; i++)
		if (irqStatus & (1u << i))
			result[i] = true;
}

void irqInit() {
	volatile uint32_t* vpic = (volatile uint32_t*)P2V(PIC_BASE);
	vpic[PIC_INT_DISABLE] = 0xFFFFFFFF;
}

