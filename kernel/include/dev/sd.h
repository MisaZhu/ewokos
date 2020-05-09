#ifndef DEV_SD_H
#define DEV_SD_H

#include <_types.h>

extern int32_t sd_init(void);
extern int32_t sd_dev_read(int32_t sector);
extern int32_t sd_dev_read_done(void* buf);
extern int32_t sd_dev_write(int32_t sector, const void* buf);
extern int32_t sd_dev_write_done(void);
extern void    sd_dev_handle(void);

#endif
