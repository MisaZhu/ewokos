#include <bsp/bsp_sd.h>
#include <stdint.h>
#include <sysinfo.h>
#include <string.h>
#include <ewoksys/syscall.h>
#include <sd/sd.h>
#include <arch/bcm283x/sd.h>

int bsp_sd_init(void) {
  sys_info_t sysinfo;
  syscall1(SYS_GET_SYS_INFO, (ewokos_addr_t)&sysinfo);
  if(strstr(sysinfo.machine, "pi4") || strstr(sysinfo.machine, "cm4"))
      return sd_init(emmc2_init, emmc2_read_sector, emmc2_write_sector);
  else
    return sd_init(bcm283x_sd_init, bcm283x_sd_read_sector, bcm283x_sd_write_sector);

}

