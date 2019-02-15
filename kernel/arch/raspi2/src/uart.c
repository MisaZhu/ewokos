#include <dev/uart.h>
#include <hardware.h>
#include <mm/mmu.h>
#include <types.h>

enum {
	// The GPIO registers base address.
	GPIO_BASE_OFF = 0x00200000, 

	// The offsets for reach register.

	// Controls actuation of pull up/down to ALL GPIO pins.
	GPPUD = (GPIO_BASE_OFF + 0x94),

	// Controls actuation of pull up/down for specific GPIO pin.
	GPPUDCLK0 = (GPIO_BASE_OFF + 0x98),

	// The base address for UART.
	UART0_BASE_OFF = 0x00201000, 

	// The offsets for reach register for the UART.
	UART0_DR = (UART0_BASE_OFF + 0x00),
	UART0_RSRECR = (UART0_BASE_OFF + 0x04),
	UART0_FR = (UART0_BASE_OFF + 0x18),
	UART0_ILPR = (UART0_BASE_OFF + 0x20),
	UART0_IBRD = (UART0_BASE_OFF + 0x24),
	UART0_FBRD = (UART0_BASE_OFF + 0x28),
	UART0_LCRH = (UART0_BASE_OFF + 0x2C),
	UART0_CR = (UART0_BASE_OFF + 0x30),
	UART0_IFLS = (UART0_BASE_OFF + 0x34),
	UART0_IMSC = (UART0_BASE_OFF + 0x38),
	UART0_RIS = (UART0_BASE_OFF + 0x3C),
	UART0_MIS = (UART0_BASE_OFF + 0x40),
	UART0_ICR = (UART0_BASE_OFF + 0x44),
	UART0_DMACR = (UART0_BASE_OFF + 0x48),
	UART0_ITCR = (UART0_BASE_OFF + 0x80),
	UART0_ITIP = (UART0_BASE_OFF + 0x84),
	UART0_ITOP = (UART0_BASE_OFF + 0x88),
	UART0_TDR = (UART0_BASE_OFF + 0x8C),
};

static void delay(int32_t count) {
	__asm__ volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
			: : [count]"r"(count) : "cc");
}

void uartDevInit(void) {
	// Disable UART0.
	mmio_write(MMIO_BASE+UART0_CR, 0x00000000);
	// Setup the GPIO pin 14 && 15.

	// Disable pull up/down for all GPIO pins & delay for 150 cycles.
	mmio_write(MMIO_BASE+GPPUD, 0x00000000);
	delay(150);

	// Disable pull up/down for pin 14,15 & delay for 150 cycles.
	mmio_write(MMIO_BASE+GPPUDCLK0, (1 << 14) | (1 << 15));
	delay(150);

	// Write 0 to GPPUDCLK0 to make it take effect.
	mmio_write(MMIO_BASE+GPPUDCLK0, 0x00000000);

	// Clear pending interrupts.
	mmio_write(MMIO_BASE+UART0_ICR, 0x7FF);

	// Set integer & fractional part of baud rate.
	// Divider = UART_CLOCK/(16 * Baud)
	// Fraction part register = (Fractional part * 64) + 0.5
	// UART_CLOCK = 3000000; Baud = 115200.

	// Divider = 3000000 / (16 * 115200) = 1.627 = ~1.
	mmio_write(MMIO_BASE+UART0_IBRD, 1);
	// Fractional part register = (.627 * 64) + 0.5 = 40.6 = ~40.
	mmio_write(MMIO_BASE+UART0_FBRD, 40);

	// Enable FIFO & 8 bit data transmissio (1 stop bit, no parity).
	mmio_write(MMIO_BASE+UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6));

	// Mask all interrupts.
	//mmio_write(MMIO_BASE+UART0_IMSC, (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) |
	//		(1 << 8) | (1 << 9) | (1 << 10));
	mmio_write(MMIO_BASE+UART0_IMSC, (1 << 4));

	// Enable UART0, receive & transfer part of UART.
	//mmio_write(MMIO_BASE+UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));
}

void uartTransmit(char c) {
	// Wait for UART to become ready to transmit.
	while (mmio_read(MMIO_BASE+UART0_FR) & (1 << 5)) {}
	mmio_write(MMIO_BASE+UART0_DR, c);
}

int uartReceive(void) {
	return mmio_read(MMIO_BASE+UART0_DR);
}

bool uartReadyToRecv(void) {
	return !(mmio_read(MMIO_BASE+UART0_FR) & (1 << 4));
}
