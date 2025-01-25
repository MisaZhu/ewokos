#ifndef MIYOO_SD_H
#define MIYOO_SD_H

#include <stdint.h>

int32_t miyoo_sd_init(void);
int32_t miyoo_sd_read_sector(int32_t sector, void* buf);
int32_t miyoo_sd_write_sector(int32_t sector, const void* buf);

#endif
