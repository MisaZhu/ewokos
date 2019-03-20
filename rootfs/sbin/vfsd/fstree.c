#include <fstree.h>
#include <kstring.h>
#include <fsinfo.h>
#include <stdlib.h>

static uint32_t _nodeIDCounter = 0;

void fsTreeNodeInit(tree_node_t* node) {
	tree_node_init(node);
	node->id = _nodeIDCounter++;
	node->data = malloc(sizeof(fs_node_t));
	fs_node_t* fn = (fs_node_t*)node->data;
	fn->name[0] = 0;
	fn->mount = 0;
	fn->type = 0;
	fn->owner = 0;
}

tree_node_t* fsNewNode() {
	tree_node_t* ret = (tree_node_t*)malloc(sizeof(tree_node_t));
	fsTreeNodeInit(ret);
	return ret;
}

tree_node_t* fsTreeSimpleGet(tree_node_t* father, const char* name) {
	if(father == NULL || strchr(name, '/') != NULL)
		return NULL;

	tree_node_t* node = father->fChild;
	while(node != NULL) {
		const char* n = FSN(node)->name;
		if(strcmp(n, name) == 0) {
			return node;
		}
		node = node->next;
	}
	return NULL;
}

tree_node_t* fsTreeGet(tree_node_t* father, const char* name) {
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


	tree_node_t* node = father;	
	char n[NAME_MAX+1];
	int j = 0;
	for(int i=0; i<NAME_MAX; i++) {
		n[i] = name[i];
		if(n[i] == 0) {
			return fsTreeSimpleGet(node, n+j);
		}
		if(n[i] == '/') {
			n[i] = 0; 
			node = fsTreeSimpleGet(node, n+j);
			if(node == NULL)
				return NULL;
			j= i+1;
		}
	}
	return NULL;
}
	
tree_node_t* fsTreeSimpleAdd(tree_node_t* father, const char* name) {
	tree_node_t* node = fsNewNode();
	fs_node_t* data = FSN(node);
	if(node == NULL ||
			data->type != FS_TYPE_DIR ||
			strchr(name, '/') != NULL)
		return NULL;

	strncpy(data->name, name, NAME_MAX);
	tree_add(father, node);
	return node;
}

