#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sysinfo.h>
#include <ewoksys/syscall.h>
#include <ewoksys/mmio.h>
#include <arch/bcm283x/pl011_uart.h>
#include "firmware.h"

int main(int argc, char* argv[]) {
	_mmio_base = mmio_map();

	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (ewokos_addr_t)&sysinfo);
	if(strcmp(sysinfo.machine, "raspberry-pi1") == 0 ||
			strcmp(sysinfo.machine, "raspberry-pi2b") == 0)  {
		printf("bt not support with pl011_uart\n");
		return -1;
	}
	klog("bt: init pl011_uart\n");
	bcm283x_pl011_uart_init();
	sleep(1);

	klog("bt: at\n");
	const char* cmd = "AT\r\n";
	int32_t res = bcm283x_pl011_uart_write(cmd, strlen(cmd));
	klog("bt: cmd %d\n", res);
    sleep(1);
	
	klog("bt: recv\n");
	const uint32_t max = 32;
    char resp[max+1];
	uint32_t rd = 0;
	while(true) {
		if(bcm283x_pl011_uart_ready_to_recv() == 0){
			uint8_t c = bcm283x_pl011_uart_recv() & 0xFF;
			klog("bt: recv c:%d\n", c);
			if(c == '\r' || c == '\n' || rd > max)
				break;
			rd++;
		}
	}
	resp[rd] = 0;
    klog("rd: %d, resp: %s\r\n", rd, resp);
	return 0;
}
