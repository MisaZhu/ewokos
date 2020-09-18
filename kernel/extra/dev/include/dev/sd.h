#ifndef DEV_SD_H
#define DEV_SD_H

#include <stdint.h>

extern int32_t sd_init(void);
extern int32_t sd_dev_read(int32_t sector);
extern int32_t sd_dev_read_done(void* buf);

#endif
