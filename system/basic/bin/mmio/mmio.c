#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sysinfo.h>
#include <ewoksys/syscall.h>
#include <ewoksys/mmio.h>

int main(int argc, char* argv[]) {
	uint32_t *reg;

	if(argc < 3){
		printf("mmio [read/write] [address] [value]\n");
		return -1;
	}
	
	setbuf(stdout, NULL);
	_mmio_base = mmio_map();
    sys_info_t sysinfo;
    syscall1(SYS_GET_SYS_INFO, (int32_t)&sysinfo);

	uint32_t addr = strtol(argv[2], NULL, 16);
	if(addr < sysinfo.mmio.phy_base || addr >= sysinfo.mmio.phy_base + sysinfo.mmio.size){
		printf("Out of range: [0x%08lX - 0x%08lX]\n", 
				sysinfo.mmio.phy_base, sysinfo.mmio.phy_base + sysinfo.mmio.size);
		return -1;
	}
	reg = (uint32_t*)(addr - sysinfo.mmio.phy_base + _mmio_base);

	if(strcasecmp(argv[1], "read") == 0){
		int size = 1;
		if(argc > 3)
			size = strtol(argv[3], NULL, 16);


		for(int i = 0; i < size; i++){
			if(addr + i*4 >= sysinfo.mmio.phy_base + sysinfo.mmio.size)
				return -1;
			printf("0x%08lX: 0x%08lX\n", addr + i * 4, reg[i]);
		}
	}else if(argc >= 4 && strcasecmp(argv[1], "write") == 0){
			uint32_t val = atol(argv[3]);
			*reg = val;
	}else{
		printf("mmio [read/write] [address] [value]\n");
		return -1;
	}
	return 0;
}
