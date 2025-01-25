#ifndef NEZHA_SD_H
#define NEZHA_SD_H

#include <stdint.h>

int32_t nezha_sd_init(void);
int32_t nezha_sd_read_sector(int32_t sector, void* buf);
int32_t nezha_sd_write_sector(int32_t sector, const void* buf);

#endif
