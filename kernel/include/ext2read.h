#ifndef EXT2_FS_READ_H
#define EXT2_FS_READ_H

#include <types.h>
#include <partition.h>

void* sd_read_ext2(const char* fname, int32_t* sz); 

int32_t read_partition(void);
int32_t partition_get(uint32_t id, partition_t* p);

#endif
