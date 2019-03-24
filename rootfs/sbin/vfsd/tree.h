#ifndef TREE_H
#define TREE_H

#include <types.h>

typedef struct tree_node {
	uint32_t id;
	struct tree_node* father; 
	struct tree_node* fChild; /*first child*/
	struct tree_node* eChild; /*last child*/
	struct tree_node* next; /*next brother*/
	struct tree_node* prev; /*prev brother*/
	
	uint32_t size;
	void* data;
} tree_node_t;

void tree_node_init(tree_node_t* node); 

void tree_add(tree_node_t* father, tree_node_t* node);

void tree_del(tree_node_t* node, free_func_t fr);

#endif
