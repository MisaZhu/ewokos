#ifndef VIRT_SD_H
#define VIRT_SD_H

#include <stdint.h>

int32_t virt_sd_init(void);
int32_t virt_sd_read_sector(int32_t sector, void* buf);
int32_t virt_sd_write_sector(int32_t sector, const void* buf);

#endif
