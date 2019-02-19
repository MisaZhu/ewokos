#include <tree.h>
#include <kstring.h>

void treeInitNode(TreeNodeT* node) {
	node->father = NULL;
	node->fChild = NULL;
	node->eChild = NULL;
	node->next = NULL;
	node->prev = NULL;
	
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

void treeDel(TreeNodeT* node, FreeFuncT fr) {
	if(node == NULL)
		return;
	/*free children*/
	TreeNodeT* c = node->fChild;
	while(c != NULL) {
		TreeNodeT* next = c->next;
		treeDel(c, fr);
		c = next;
	}

	TreeNodeT* father = node->father;
	if(father != NULL) {
		if(father->fChild == node)
			father->fChild = node->next;
		if(father->eChild == node)
			father->eChild = node->prev;
	}

	if(node->next != NULL)
		node->next->prev = node->prev;
	if(node->prev != NULL)
		node->prev->next = node->next;

	/*free node content*/
	if(node->data != NULL)
		fr(node->data);
	fr(node);
}
