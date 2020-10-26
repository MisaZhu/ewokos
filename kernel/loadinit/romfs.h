#ifndef ROMFS_H
#define ROMFS_H

#include <stdint.h>

char* romfs_get_by_name(const char* fname, int32_t *size);
int32_t romfs_get_by_index(uint32_t index, char* name, char* data);
int32_t romfs_load(void);

#endif
