#ifndef SD_H
#define SD_H

#include <sys/ewokdef.h>
#ifdef __cplusplus
extern "C" {
#endif


int32_t sd_init_arch(void);
int32_t sd_read_sector_arch(int32_t sector, void* buf); 
int32_t sd_write_sector_arch(int32_t sector, const void* buf);

int32_t sd_read(int32_t block, void* buf); 
int32_t sd_write(int32_t block, const void* buf); 
int32_t sd_init(void);
int32_t sd_quit(void);
int32_t sd_set_buffer(uint32_t sector_num);

#define SECTOR_SIZE 512

#ifdef __cplusplus
}
#endif

#endif
