#include <kernel/system.h>
#include <kernel/hw_info.h>
#include <bcm283x/pl011_uart.h>

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

int32_t pl011_uart_init(uint32_t baud) {
	// Disable UART0.
	put32(_sys_info.mmio.v_base+UART0_CR, 0x00000000);
	// Setup the GPIO pin 14 && 15.

	// Disable pull up/down for all GPIO pins & delay for 150 cycles.
	put32(_sys_info.mmio.v_base+GPPUD, 0x00000000);
	_delay(150);

	// Disable pull up/down for pin 14,15 & delay for 150 cycles.
	put32(_sys_info.mmio.v_base+GPPUDCLK0, (1 << 14) | (1 << 15));
	_delay(150);

	// Write 0 to GPPUDCLK0 to make it take effect.
	put32(_sys_info.mmio.v_base+GPPUDCLK0, 0x00000000);

	// Clear pending interrupts.
	put32(_sys_info.mmio.v_base+UART0_ICR, 0x7FF);

	// Set integer & fractional part of baud rate.
	// Divider = UART_CLOCK/(16 * Baud)
	// Fraction part register = (Fractional part * 64) + 0.5
	// UART_CLOCK = 3000000; Baud = 115200.

	// Divider = 3000000 / (16 * 115200) = 1.627 = ~1.
	put32(_sys_info.mmio.v_base+UART0_IBRD, 1);
	// Fractional part register = (.627 * 64) + 0.5 = 40.6 = ~40.
	put32(_sys_info.mmio.v_base+UART0_FBRD, 40);

	// Enable FIFO & 8 bit data transmissio (1 stop bit, no parity).
	put32(_sys_info.mmio.v_base+UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6));

	// Mask all interrupts.
	put32(_sys_info.mmio.v_base+UART0_IMSC, (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) |
			(1 << 8) | (1 << 9) | (1 << 10));

	// Enable UART0, receive & transfer part of UART.
	put32(_sys_info.mmio.v_base+UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));
	return 0;
}

static void uart_trans(char c) {
	// Wait for UART to become ready to transmit.
	while (get32(_sys_info.mmio.v_base+UART0_FR) & (1 << 5)) {}
	if(c == '\r')
		c = '\n';
	put32(_sys_info.mmio.v_base+UART0_DR, c);
}

int32_t pl011_uart_write(const void* data, uint32_t size) {
  int32_t i;
  for(i=0; i<(int32_t)size; i++) {
    char c = ((char*)data)[i];
    uart_trans(c);
  }
  return i;
}

