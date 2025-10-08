#include <ewoksys/mmio.h>

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

static inline void delay(int32_t n) {
	while(n > 0) n--;
}

int32_t bcm283x_pl011_uart_init(void) {
	// Disable UART0.
	put32(_mmio_base+UART0_CR, 0x00000000);
	// Setup the GPIO pin 14 && 15.

	// Disable pull up/down for all GPIO pins & delay for 150 cycles.
	put32(_mmio_base+GPPUD, 0x00000000);
	delay(150);

	// Disable pull up/down for pin 14,15 & delay for 150 cycles.
	put32(_mmio_base+GPPUDCLK0, (1 << 14) | (1 << 15));
	delay(150);

	// Write 0 to GPPUDCLK0 to make it take effect.
	put32(_mmio_base+GPPUDCLK0, 0x00000000);

	// Clear pending interrupts.
	put32(_mmio_base+UART0_ICR, 0x7FF);

	// Set integer & fractional part of baud rate.
	// Divider = UART_CLOCK/(16 * Baud)
	// Fraction part register = (Fractional part * 64) + 0.5
	// UART_CLOCK = 3000000; Baud = 115200.

	put32(_mmio_base+UART0_IBRD, 1); // Divider = 3000000 / (16 * 115200) = 1.627 = ~1.
	put32(_mmio_base+UART0_FBRD, 40); // Fractional part register = (.627 * 64) + 0.5 = 40.6 = ~40.

	// 2. 配置波特率：3000000 = 250MHz / (16 * (IBRD + FBRD/64))
    //put32(_mmio_base+UART0_IBRD, 5);    // 整数部分：250000000 / (16*3000000) ≈ 5.208 → 5
    //put32(_mmio_base+UART0_FBRD, 13);   // 小数部分：0.208 * 64 ≈ 13.312 → 13

	// Enable FIFO & 8 bit data transmissio (1 stop bit, no parity).
	put32(_mmio_base+UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6));

	// Mask all interrupts.
	/*put32(_mmio_base+UART0_IMSC,
			(1 << 1) | (1 << 4) | (1 << 5) | 
			(1 << 6) | (1 << 7) |
			(1 << 8) | (1 << 9) | (1 << 10));
			*/

	// Enable UART0, receive & transfer part of UART.
	put32(_mmio_base+UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));
	return 0;
}

static inline int32_t bcm283x_pl011_uart_ready_to_send(void) {
	if(get32(_mmio_base+UART0_FR) & (1 << 5))
		return -1;
	return 0;
}

static inline int32_t bcm283x_pl011_uart_ready_to_recv(void) {
	if(get32(_mmio_base+UART0_FR) & (1 << 4)) 
		return -1;
	return 0;
}

int32_t bcm283x_pl011_uart_send(uint8_t c) {
	// Wait for UART to become ready to transmit.
	while(bcm283x_pl011_uart_ready_to_send() != 0) {
		usleep(1000);
	}
	put32(_mmio_base+UART0_DR, c);
	return 0;
}

int32_t bcm283x_pl011_uart_recv(uint32_t timeout_ms) {
	uint32_t ms = 0;
	while(bcm283x_pl011_uart_ready_to_recv() != 0) {
		usleep(1000);
		ms++;
		if(ms > timeout_ms)
			return -1;
	}
	return get32(_mmio_base+UART0_DR);
}

int32_t bcm283x_pl011_uart_write(const void* data, uint32_t size) {
  int32_t i;
  for(i=0; i<(int32_t)size; i++) {
    char c = ((char*)data)[i];
    bcm283x_pl011_uart_send(c);
  }
  return i;
}

