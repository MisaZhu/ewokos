#include <dev/uart.h>
#include <mm/mmu.h>

#define REG32(x) (*(volatile uint32_t*)(x))

//#undef MMIO_BASE
//#define MMIO_BASE 0xFF000000

#define UART_LSR_THRE	0x20

int32_t uart_dev_init(uint32_t baud) {
	return 0;
}

int32_t uart_write(const void* data, uint32_t size) {
	for(int i = 0; i <  (int)size; i++){
	char ch = ((char*)data)[i];
		while (((REG32(_sys_info.mmio.v_base + 0x10060014)) & UART_LSR_THRE) == 0);	
			REG32(_sys_info.mmio.v_base + 0x10060000) = ch;
	}
	return 0;
}

