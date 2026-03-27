#ifndef BCM283x_SD_H
#define BCM283x_SD_H

#include <stdint.h>

int32_t bcm283x_sd_init(void);
int32_t bcm283x_sd_read_sector(int32_t sector, void* buf, uint32_t cnt);
int32_t bcm283x_sd_write_sector(int32_t sector, const void* buf, uint32_t cnt);

int32_t emmc2_init(void);
int32_t emmc2_read_sector(int32_t sector, void* buf);
int32_t emmc2_write_sector(int32_t sector, const void* buf);

#endif
