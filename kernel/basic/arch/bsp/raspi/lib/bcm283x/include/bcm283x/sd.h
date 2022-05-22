#ifndef BCM283x_SD_H
#define BCM283x_SD_H

#include <stdint.h>
int32_t bcm283x_sd_init(void);
int32_t bcm283x_sd_read(int32_t sector); 
int32_t bcm283x_sd_read_done(void* buf);
int32_t bcm283x_sd_write(int32_t sector, const void* buf);
int32_t bcm283x_sd_write_done(void);

#endif
