#include <bsp/bsp_sd.h>
#include <stdint.h>
#include <sysinfo.h>
#include <string.h>
#include <ewoksys/syscall.h>
#include <sd/sd.h>
#include <ewoksys/mmio.h>
#include <arch/bcm283x/sd.h>


void* cache_entry[4096] = {0};

static int32_t bsp_sd_read_cache(uint32_t sector, void *buf){
	uint32_t l1 = (sector >> 21) & 0x1FF;
	if(cache_entry[l1] == 0)
		cache_entry[l1] = calloc(4096, 1);

	uint32_t *l2_entry = cache_entry[l1];
	uint32_t l2 = (sector >> 12) & 0x1FF;
	if(l2_entry[l2] == 0)
		l2_entry[l2] = calloc(4096, 1);

	uint32_t *l3_entry = l2_entry[l2];
	uint32_t l3 = (sector >> 3) & 0x1FF;
	if(l3_entry[l3] == 0){
		l3_entry[l3] = malloc(4096);
		mmc_read_blocks(l3_entry[l3], sector&(~0x7), 8);
	}
	uint8_t *page = l3_entry[l3];
	memcpy(buf, page + (sector & 0x7) * 512, 512);
	return 0;
}

static int32_t bsp_sd_write_cache(uint32_t sector, void *buf){
	/*uint32_t l1 = (sector >> 21) & 0x1FF;
	if(cache_entry[l1] == 0)
		cache_entry[l1] = calloc(4096, 1);

	uint32_t *l2_entry = cache_entry[l1];
	uint32_t l2 = (sector >> 12) & 0x1FF;
	if(l2_entry[l2] == 0)
		l2_entry[l2] = calloc(4096, 1);

	uint32_t *l3_entry = l2_entry[l2];
	uint32_t l3 = (sector >> 3) & 0x1FF;
	if(l3_entry[l3] == 0){
		l3_entry[l3] = malloc(4096);
		mmc_read_blocks(l3_entry[l3], sector&(~0x7),1);
	}
	uint8_t *page = l3_entry[l3];
	memcpy(page + (sector & 0x7) * 512, buf, 512);
	// Write back the modified page to SD card
	mmc_write_blocks(sector&(~0x7), 1, page);
	*/

	mmc_write_blocks(sector, 1, buf);
	return 0;
}

int32_t bsp_sd_init_by_info(void) {
   sys_info_t sysinfo;
    _mmio_base = mmio_map();
   syscall1(SYS_GET_SYS_INFO, (ewokos_addr_t)&sysinfo);
   if(strstr(sysinfo.machine, "pi4") || strstr(sysinfo.machine, "cm4"))
        return mmc_init(1);
   else
        return mmc_init(0);
}

int bsp_sd_init(void) {
    return sd_init(bsp_sd_init_by_info, bsp_sd_read_cache, bsp_sd_write_cache);
}

