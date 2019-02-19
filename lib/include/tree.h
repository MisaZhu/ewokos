#ifndef TREE_H
#define TREE_H

#include <types.h>

typedef struct TreeNode {
	struct TreeNode* father; 
	struct TreeNode* fChild; /*first child*/
	struct TreeNode* eChild; /*last child*/
	struct TreeNode* next; /*next brother*/
	struct TreeNode* prev; /*prev brother*/
	
	void* data;
} TreeNodeT;

void treeInitNode(TreeNodeT* node); 

void treeAdd(TreeNodeT* father, TreeNodeT* node);

void treeDel(TreeNodeT* node, FreeFuncT fr);

#endif
