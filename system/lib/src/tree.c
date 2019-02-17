#include <tree.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <kserv/fs.h>

void treeInitNode(TreeNodeT* node) {
	node->father = NULL;
	node->fChild = NULL;
	node->eChild = NULL;
	node->next = NULL;
	node->prev = NULL;
	
	node->freeFunc = NULL;
	node->data = NULL;
}

void treeAdd(TreeNodeT* father, TreeNodeT* node) {
	if(father == NULL || node == NULL)
		return;

	node->father = father;
	if(father->eChild == NULL) {
		father->fChild = node;
	}
	else {
		father->eChild->next = node;
		node->prev = father->eChild;
	}
	father->eChild = node;
}

void treeDel(TreeNodeT* node) {
	if(node == NULL)
		return;
	/*free children*/
	TreeNodeT* c = node->fChild;
	while(c != NULL) {
		TreeNodeT* next = c->next;
		treeDel(c);
		c = next;
	}

	/*free node content*/
	if(node->data != NULL && node->freeFunc != NULL)
		node->freeFunc(node->data);
	free(node);
}
