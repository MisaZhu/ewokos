#ifndef DEV_SD_H
#define DEV_SD_H
#include <stdint.h>
int32_t sd_dev_read(int32_t sector);
int32_t sd_dev_read_done(void* buf);
int32_t sd_dev_read_blocks(int32_t sector, void* buf, uint32_t count);
#endif
