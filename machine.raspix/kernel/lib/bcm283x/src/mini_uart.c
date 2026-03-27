#include <mm/mmu.h>
#include <bcm283x/gpio.h>
#include <bcm283x/mini_uart.h>
#include <kernel/hw_info.h>

#define AUX_OFFSET 0x00215000
#define UART_OFFSET 0x00215040

#define AUX_BASE (_sys_info.mmio.v_base | AUX_OFFSET)
#define UART_BASE (_sys_info.mmio.v_base | UART_OFFSET)

#define AUX_ENABLES (AUX_BASE+0x04)
#define UART_AUX_ENABLE 0x01

#define UART_IO_REG   (UART_BASE+0x00)
#define UART_IER_REG  (UART_BASE+0x04)
#define UART_IIR_REG  (UART_BASE+0x08)
#define UART_LCR_REG  (UART_BASE+0x0C)
#define UART_MCR_REG  (UART_BASE+0x10)
#define UART_LSR_REG  (UART_BASE+0x14)
#define UART_MSR_REG  (UART_BASE+0x18)
#define UART_SCRATCH  (UART_BASE+0x1C)
#define UART_CNTL_REG (UART_BASE+0x20)
#define UART_STAT_REG (UART_BASE+0x24)
#define UART_BAUD_REG (UART_BASE+0x28)

#define UART_BAUD_9600 3254
#define UART_BAUD_19200 1628
#define UART_BAUD_115200 270
#define UART_BAUD_DEFAULT UART_BAUD_115200

#define UART_TXD_GPIO 14
#define UART_RXD_GPIO 15

static inline uint32_t get_baud(uint32_t uart_baud) {
	if(uart_baud == 115200)
		return UART_BAUD_115200;
	else if(uart_baud == 19200)
		return UART_BAUD_19200;
	else if(uart_baud == 9600)
		return UART_BAUD_9600;
	return UART_BAUD_DEFAULT;
}

int32_t mini_uart_init(uint32_t baud) {
	unsigned int data = get32(AUX_ENABLES);
	/* enable uart */
	put32(AUX_ENABLES, data|UART_AUX_ENABLE);
	/* configure uart */
	put32(UART_LCR_REG, 0x03); /** 8-bit data (errata in manual 0x01) */
	put32(UART_MCR_REG, 0x00);
	put32(UART_IER_REG, 0x00); /** no need interrupt */
	/** check requested baudrate **/
	/** baudrate count = ((sys_clk/baudrate)/8)-1 */
	put32(UART_BAUD_REG, get_baud(baud)); /** 16-bit baudrate counter */
	/* disable pull-down default on tx/rx pins */
	bcm283x_gpio_pull(UART_TXD_GPIO, bcm283x_gpio_pull_NONE);
	bcm283x_gpio_pull(UART_RXD_GPIO, bcm283x_gpio_pull_NONE);
	/* setup uart TX1/RX1 at pins 14/15 (ALTF5) */
	bcm283x_gpio_config(UART_TXD_GPIO, GPIO_ALTF5);
	bcm283x_gpio_config(UART_RXD_GPIO, GPIO_ALTF5);
	/** ready to go? */
	put32(UART_IIR_REG, 0xC6); /** clear TX/RX FIFO */
	put32(UART_CNTL_REG, 0x03); /** enable TX/RX */
	return 0;
}

#define UART_TXFIFO_EMPTY 0x20
#define UART_RXFIFO_AVAIL 0x01

static void uart_trans(uint32_t data) {
	while(!(get32(UART_LSR_REG) & UART_TXFIFO_EMPTY));
	if(data == '\r')
		data = '\n';
	put32(UART_IO_REG, data);
}

int32_t mini_uart_write(const void* data, uint32_t size) {
  int32_t i;
  for(i=0; i<(int32_t)size; i++) {
    char c = ((char*)data)[i];
    uart_trans(c);
  }
  return i;
}

