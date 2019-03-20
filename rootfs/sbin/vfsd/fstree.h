#ifndef FS_TREE_H
#define FS_TREE_H

#include "types.h"
#include "tree.h"
#include "fsinfo.h"

void fsTreeInit();

void fsTreeNodeInit(tree_node_t* node);

tree_node_t* fsNewNode();

tree_node_t* fsTreeSimpleGet(tree_node_t* father, const char* name);

tree_node_t* fsTreeGet(tree_node_t* father, const char* name);

tree_node_t* fsTreeSimpleAdd(tree_node_t* father, const char* name);

#endif
