#include <dev/uart.h>
#include <mm/mmu.h>

#define REG32(x) (*(volatile uint32_t*)(x))

//#undef MMIO_BASE
//#define MMIO_BASE 0xFF000000

uint32_t uart_base = MMIO_BASE;
#define UART_LSR_THRE	0x20

int32_t uart_dev_init(void) {
	return 0;
}

int32_t uart_write(const void* data, uint32_t size) {
	for(int i = 0; i <  (int)size; i++){
	char ch = ((char*)data)[i];
		while (((REG32(MMIO_BASE + 0x160014)) & UART_LSR_THRE) == 0);	
			REG32(MMIO_BASE + 0x160000) = ch;
	}
	return 0;
}

