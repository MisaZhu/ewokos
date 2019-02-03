#ifndef FSINFO_H
#define FSINFO_H

#include <types.h>

#define FS_TYPE_DIR 0x0
#define FS_TYPE_FILE 0x01
#define FS_TYPE_DEV_FILE 0x02

typedef struct FSInfo {
	uint32_t size;
	uint32_t type;
	uint32_t owner;
	char name[FNAME_MAX];
} FSInfoT;

#endif
