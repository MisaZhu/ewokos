#ifndef TREE_H
#define TREE_H

#include <types.h>

#define NAME_MAX 127

#define FS_FLAG_MNT_ROOT 0x01

typedef struct TreeNode {
	struct TreeNode* father; 
	struct TreeNode* fChild; /*first child*/
	struct TreeNode* eChild; /*last child*/
	struct TreeNode* next; /*next brother*/
	struct TreeNode* prev; /*prev brother*/

	char name[NAME_MAX+1];
	int32_t mount;
	uint32_t type;
	uint32_t flags;
	uint32_t owner;
} TreeNodeT;

void treeInitNode(TreeNodeT* node); 

void treeAdd(TreeNodeT* father, TreeNodeT* node);

TreeNodeT* treeSimpleGet(TreeNodeT* father, const char* name);

TreeNodeT* treeGet(TreeNodeT* father, const char* name);

TreeNodeT* treeSimpleAdd(TreeNodeT* father, const char* name);

TreeNodeT* mount(TreeNodeT* to, uint32_t type, const char* device);

void unmount(TreeNodeT* node);

void treeDel(TreeNodeT* node);

#endif
