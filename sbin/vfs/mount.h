#ifndef MOUNT_H
#define MOUNT_H

#include <types.h>

struct TreeNode;

typedef struct {
	uint32_t type;
	char device[NAME_MAX+1];

	struct TreeNode* to;
} MountT;


#endif
