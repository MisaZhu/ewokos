#ifndef VERSATILEPB_SD_H
#define VERSATILEPB_SD_H

#include <stdint.h>
int32_t versatilepb_sd_init(void);
int32_t versatilepb_sd_read_sector(int32_t sector, void* buf); 
int32_t versatilepb_sd_write_sector(int32_t sector, const void* buf); 
	
#endif
