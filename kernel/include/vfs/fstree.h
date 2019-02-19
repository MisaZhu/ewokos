#ifndef FS_TREE_H
#define FS_TREE_H

#include "types.h"
#include "tree.h"

#define FS_FLAG_MNT_ROOT 0x01

typedef struct FSNode {
	uint32_t id;
	char name[NAME_MAX];
	int32_t mount;
	uint32_t type;
	uint32_t flags;
	uint32_t owner;
} FSNodeT;

#define FSN(n) ((FSNodeT*)((n)->data))

void fsTreeNodeInit(TreeNodeT* node);

TreeNodeT* fsNewNode();

TreeNodeT* fsTreeSimpleGet(TreeNodeT* father, const char* name);

TreeNodeT* fsTreeGet(TreeNodeT* father, const char* name);

TreeNodeT* fsTreeSimpleAdd(TreeNodeT* father, const char* name);


#endif
