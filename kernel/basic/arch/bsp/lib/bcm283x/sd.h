#ifndef BCM283X_SD_H
#define BCM283X_SD_H

#include <stdint.h>

extern int32_t bcm283x_sd_init(void);
extern int32_t bcm283x_sd_read(int32_t sector);
extern int32_t bcm283x_sd_read_done(void* buf);

#endif
