#include <irq.h>
#include <mm/mmu.h>

#include <timer.h>

/* memory mapping for the prime interrupt controller */
#define PIC ((volatile uint32_t*)(MMIO_BASE+0x00140000))
/* interrupt controller register offsets */
#define PIC_STATUS     0
#define PIC_INT_ENABLE 4

/* memory mapping for the slave interrupt controller */
#define SIC ((volatile uint32_t*)(MMIO_BASE+0x00003000))
/* interrupt controller register offsets */
#define SIC_STATUS     0
#define SIC_INT_ENABLE 2


#define PINT_TIMER0 (1 << 4)
#define PINT_UART0 (1 << 12)
#define PINT_SIC (1 << 31)

#define SINT_KEY (1 << 3)
#define SINT_MOUSE (1 << 4)
#define SINT_SDC (1 << 22)

void irq_init() {
	PIC[PIC_INT_ENABLE] |= PINT_TIMER0; //timer 0,1
	PIC[PIC_INT_ENABLE] |= PINT_UART0; //uart0
	PIC[PIC_INT_ENABLE] |= PINT_SIC; //SIC on

	SIC[SIC_INT_ENABLE] |= SINT_KEY; //keyboard.
	SIC[SIC_INT_ENABLE] |= SINT_MOUSE; //mouse.
	SIC[SIC_INT_ENABLE] |= SINT_SDC; //SD card.
}

void keyboard_handle();
void mouse_handle();
void uart_handle();

void irq_handle() {
	uint32_t pic_status = PIC[PIC_STATUS];
	uint32_t sic_status = SIC[SIC_STATUS];
	if((pic_status & PINT_TIMER0) != 0) {
		timer_handle();
	}
	if((pic_status & PINT_UART0) != 0) {
		uart_handle();
	}
	if((pic_status & PINT_SIC) != 0) {
		if((sic_status & SINT_KEY) != 0) {
			keyboard_handle();
		}
		if((sic_status & SINT_MOUSE) != 0) {
			mouse_handle();
		}
		if((sic_status & SINT_SDC) != 0) {
			sdc_handle();
		}
	}
}
