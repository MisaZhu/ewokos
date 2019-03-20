#include <tree.h>
#include <kstring.h>

void tree_node_init(tree_node_t* node) {
	memset(node, 0, sizeof(tree_node_t));
}

void tree_add(tree_node_t* father, tree_node_t* node) {
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
	father->size++;
	father->eChild = node;
}

void tree_del(tree_node_t* node, free_func_t fr) {
	if(node == NULL)
		return;
	/*free children*/
	tree_node_t* c = node->fChild;
	while(c != NULL) {
		tree_node_t* next = c->next;
		tree_del(c, fr);
		c = next;
	}

	tree_node_t* father = node->father;
	if(father != NULL) {
		if(father->fChild == node)
			father->fChild = node->next;
		if(father->eChild == node)
			father->eChild = node->prev;
		father->size--;
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
