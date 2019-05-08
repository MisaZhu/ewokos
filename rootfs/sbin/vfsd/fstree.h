#ifndef FS_TREE_H
#define FS_TREE_H

#include "types.h"
#include "tree.h"
#include "tstr.h"

typedef struct FSNode {
	tstr_t* name;
	int32_t mount;
	uint32_t size;
	uint32_t type;
	int32_t owner;
	void* data;
} fs_node_t;

#define FSN(n) ((fs_node_t*)((n)->data))

void fs_tree_init();

void fs_tree_node_init(tree_node_t* node);

void fs_tree_del(tree_node_t* node);

tree_node_t* fs_new_node();

tree_node_t* fs_tree_simple_get(tree_node_t* father, const char* name);

tree_node_t* fs_tree_get(tree_node_t* father, const char* name);

tree_node_t* fs_tree_simple_add(tree_node_t* father, const char* name);

#endif
