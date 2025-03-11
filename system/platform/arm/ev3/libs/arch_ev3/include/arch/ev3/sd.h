#ifndef EV3_SD_H
#define EV3_SD_H

#include <stdint.h>

int32_t ev3_sd_init(void);
int32_t ev3_sd_read_sector(int32_t sector, void* buf);
int32_t ev3_sd_write_sector(int32_t sector, const void* buf);

#endif
