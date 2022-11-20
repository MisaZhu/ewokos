#include <bsp/bsp_sd.h>
#include <stdint.h>
#include <sysinfo.h>
#include <string.h>
#include <sys/syscall.h>
#include <sd/sd.h>
#include <arch/bcm283x/sd.h>
#include <arch/miyoo/sd.h>
#include <arch/rk3326/sd.h>
#include <arch/rk3128/sd.h>
#include <arch/virt/sd.h>
#include <arch/vpb/sd.h>

int bsp_sd_init(void) {
    int res = -1;
	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (int32_t)&sysinfo);
	if(strncmp(sysinfo.machine, "raspi", 5) == 0)
		res = sd_init(bcm283x_sd_init, bcm283x_sd_read_sector, bcm283x_sd_write_sector);
	else if(strncmp(sysinfo.machine, "miyoo-mini", 10) == 0)
		res = sd_init(miyoo_sd_init, miyoo_sd_read_sector, miyoo_sd_write_sector);
	else if(strcmp(sysinfo.machine, "versatilepb") == 0)
		res = sd_init(versatilepb_sd_init, versatilepb_sd_read_sector, versatilepb_sd_write_sector);
	else if(strncmp(sysinfo.machine, "rk3326", 6) == 0)
		res = sd_init(rk3326_sd_init, rk3326_sd_read_sector, rk3326_sd_write_sector);
	else if(strncmp(sysinfo.machine, "rk3128", 6) == 0)
		res = sd_init(rk3128_sd_init, rk3128_sd_read_sector, rk3128_sd_write_sector);
	else if(strncmp(sysinfo.machine, "virt", 4) == 0){
		res = sd_init(virt_sd_init, virt_sd_read_sector, virt_sd_write_sector);
	}
    
    return res;
}
