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
	node->name[0] = 0;
	node->mount = -1;
	node->type = 0;
	node->flags = 0;
}

TreeNodeT* treeSimpleGet(TreeNodeT* father, const char* name) {
	if(father == NULL || strchr(name, '/') != NULL)
		return NULL;

	TreeNodeT* node = father->fChild;
	while(node != NULL) {
		if(strcmp(node->name, name) == 0) {
			return node;
		}
		node = node->next;
	}
	return NULL;
}

TreeNodeT* treeGet(TreeNodeT* father, const char* name) {
	if(father == NULL)
		return NULL;
	
	if(name[0] == '/') {
		/*go to root*/
		while(father->father != NULL)
			father = father->father;

		name = name+1;
		if(name[0] == 0)
			return father;
	}


	TreeNodeT* node = father;	
	char n[NAME_MAX+1];
	int j = 0;
	for(int i=0; i<NAME_MAX; i++) {
		n[i] = name[i];
		if(n[i] == 0) {
			return treeSimpleGet(node, n+j);
		}
		if(n[i] == '/') {
			n[i] = 0; 
			node = treeSimpleGet(node, n+j);
			if(node == NULL)
				return NULL;
			j= i+1;
		}
	}
	return NULL;
}
	
TreeNodeT* treeSimpleAdd(TreeNodeT* father, const char* name) {
	TreeNodeT* node = (TreeNodeT*)malloc(sizeof(TreeNodeT));
	if(node == NULL ||
			node->type != FS_TYPE_DIR ||
			strchr(name, '/') != NULL)
		return NULL;

	treeInitNode(node);
	strncpy(node->name, name, NAME_MAX);
	treeAdd(father, node);
	return node;
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
	free(node);
}
