#ifndef SD_H
#define SD_H

#include <ewoksys/ewokdef.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t (*sd_init_func)(void);
typedef int32_t (*sd_read_sector_func)(int32_t sector, void* buf);
typedef int32_t (*sd_read_sectors_func)(int32_t sector, void* buf, uint32_t count);
typedef int32_t (*sd_write_sector_func)(int32_t sector, const void* buf);

int32_t sd_read_sector(int32_t sector, void* buf);
int32_t sd_read_sectors(int32_t sector, void* buf, uint32_t count);
int32_t sd_write_sector(int32_t sector, const void* buf); 
int32_t sd_read(int32_t block, void* buf); 
int32_t sd_read_blocks(int32_t block, void* buf, uint32_t count);
int32_t sd_write(int32_t block, const void* buf); 
int32_t sd_init(sd_init_func init, sd_read_sector_func rd, sd_write_sector_func wr);
int32_t sd_init_ex(sd_init_func init, sd_read_sector_func rd, sd_read_sectors_func rds, sd_write_sector_func wr);
int32_t sd_quit(void);
int32_t sd_set_max_sector_index(uint32_t sector_num);
void    sd_set_buffer_size(uint32_t size);
void    sd_enable_sector_buffer(int enabled);

#define SECTOR_SIZE 512

#ifdef __cplusplus
}
#endif

#endif
