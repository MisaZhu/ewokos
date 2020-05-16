#ifndef SDINIT_H
#define SDINIT_H

#include <stdint.h>

int32_t sdinit_read(int32_t block, void* buf); 
int32_t sdinit_init(void);
int32_t sdinit_quit(void);

#define SECTOR_SIZE 512

#endif
