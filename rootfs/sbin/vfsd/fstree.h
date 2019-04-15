#ifndef FS_TREE_H
#define FS_TREE_H

#include "types.h"
#include "tree.h"
#include "fsinfo.h"

void fs_tree_init();

void fs_tree_node_init(tree_node_t* node);

tree_node_t* fs_new_node();

tree_node_t* fs_tree_simple_get(tree_node_t* father, const char* name);

tree_node_t* fs_tree_get(tree_node_t* father, const char* name);

tree_node_t* fs_tree_simple_add(tree_node_t* father, const char* name);

#endif
