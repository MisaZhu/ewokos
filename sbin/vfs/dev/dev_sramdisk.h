#ifndef DEV_SRAMDISK_H
#define DEV_SRAMDISK_H

#include <types.h>
#include <tree.h>

void mountSRamDisk(TreeNodeT* node);

int readSRamDisk(TreeNodeT* node, int seek, char* buf, uint32_t size);

#endif
