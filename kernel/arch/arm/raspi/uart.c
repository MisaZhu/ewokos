#include <dev/uart.h>
#include <buffer.h>
#include <mm/mmu.h>
#include "gpio_arch.h"
#include <kernel/system.h>

#define AUX_OFFSET 0x00215000
#define UART_OFFSET 0x00215040

#define AUX_BASE (_mmio_base | AUX_OFFSET)
#define UART_BASE (_mmio_base | UART_OFFSET)

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

#define UART_BAUD_115200 270
#define UART_BAUD_9600 3254
#define UART_BAUD_DEFAULT UART_BAUD_115200

#define UART_TXD_GPIO 14
#define UART_RXD_GPIO 15


static void gpio_config(int32_t gpio_num, int32_t gpio_sel) {
	uint32_t raddr = (uint32_t)GPIO_FSEL0 + ((gpio_num/10)<<2);
	uint32_t shift = (gpio_num%10) * GPIO_SEL_BITS;
	uint32_t value = gpio_sel << shift;
	uint32_t mask = GPIO_SEL << shift;
	uint32_t data = get32(raddr);
	data &= ~mask;
	data |= value;
	put32(raddr, data);
}

static void gpio_pull(int32_t gpio_num, int32_t pull_dir) {
	uint32_t shift = (gpio_num % 32);
	uint32_t index = (gpio_num/32) + 1;
	*GPIO_PUD = pull_dir & GPIO_PULL_MASK;
	_delay(150);
	put32((uint32_t)GPIO_PUD+(index<<2), 1<<shift); /* enable ppud clock */
	_delay(150);
	*GPIO_PUD = GPIO_PULL_NONE;
	put32((uint32_t)GPIO_PUD+(index<<2), 0); /* disable ppud clock */
}

int32_t uart_dev_init(void) {
	unsigned int data = get32(AUX_ENABLES);
	/* enable uart */
	put32(AUX_ENABLES, data|UART_AUX_ENABLE);
	/* configure uart */
	put32(UART_LCR_REG, 0x03); /** 8-bit data (errata in manual 0x01) */
	put32(UART_MCR_REG, 0x00);
	put32(UART_IER_REG, 0x00); /** no need interrupt */
	/** check requested baudrate **/
	/** baudrate count = ((sys_clk/baudrate)/8)-1 */
	put32(UART_BAUD_REG, UART_BAUD_DEFAULT); /** 16-bit baudrate counter */
	/* disable pull-down default on tx/rx pins */
	gpio_pull(UART_TXD_GPIO, GPIO_PULL_NONE);
	gpio_pull(UART_RXD_GPIO, GPIO_PULL_NONE);
	/* setup uart TX1/RX1 at pins 14/15 (ALTF5) */
	gpio_config(UART_TXD_GPIO, GPIO_ALTF5);
	gpio_config(UART_RXD_GPIO, GPIO_ALTF5);
	/** ready to go? */
	put32(UART_IIR_REG, 0xC6); /** clear TX/RX FIFO */
	put32(UART_CNTL_REG, 0x03); /** enable TX/RX */
	return 0;
}

#define UART_TXFIFO_EMPTY 0x20
#define UART_RXFIFO_AVAIL 0x01

void uart_trans(uint32_t data) {
	while(!(get32(UART_LSR_REG) & UART_TXFIFO_EMPTY));
	if(data == '\r')
		data = '\n';
	put32(UART_IO_REG, data);
}

int32_t uart_write(dev_t* dev, const void* data, uint32_t size) {
  (void)dev;
  int32_t i;
  for(i=0; i<(int32_t)size; i++) {
    char c = ((char*)data)[i];
    uart_trans(c);
  }
  return i;
}

