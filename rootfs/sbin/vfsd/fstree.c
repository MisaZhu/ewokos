#include <fstree.h>
#include <kstring.h>
#include <fsinfo.h>
#include <stdlib.h>

static uint32_t _node_id_counter = 0;

void fs_tree_node_init(tree_node_t* node) {
	tree_node_init(node);
	node->id = _node_id_counter++;
	node->data = malloc(sizeof(fs_node_t));
	fs_node_t* fn = (fs_node_t*)node->data;
	fn->name = tstr_new("", malloc, free);
	fn->mount = 0;
	fn->type = 0;
	fn->owner = 0;
	fn->data = NULL;
}

tree_node_t* fs_new_node() {
	tree_node_t* ret = (tree_node_t*)malloc(sizeof(tree_node_t));
	fs_tree_node_init(ret);
	return ret;
}

tree_node_t* fs_tree_simple_get(tree_node_t* father, const char* name) {
	if(father == NULL || strchr(name, '/') != NULL)
		return NULL;

	tree_node_t* node = father->fChild;
	while(node != NULL) {
		const char* n = CS(FSN(node)->name);
		if(strcmp(n, name) == 0) {
			return node;
		}
		node = node->next;
	}
	return NULL;
}

tree_node_t* fs_tree_get(tree_node_t* father, const char* name) {
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
	char n[FULL_NAME_MAX+1];
	int j = 0;
	for(int i=0; i<FULL_NAME_MAX; i++) {
		n[i] = name[i];
		if(n[i] == 0) {
			return fs_tree_simple_get(node, n+j);
		}
		if(n[i] == '/') {
			n[i] = 0; 
			node = fs_tree_simple_get(node, n+j);
			if(node == NULL)
				return NULL;
			j= i+1;
		}
	}
	return NULL;
}
	
tree_node_t* fs_tree_simple_add(tree_node_t* father, const char* name) {
	tree_node_t* node = fs_new_node();
	fs_node_t* data = FSN(node);
	if(node == NULL ||
			data->type != FS_TYPE_DIR ||
			strchr(name, '/') != NULL)
		return NULL;

	tstr_cpy(data->name, name);
	tree_add(father, node);
	return node;
}

void fs_tree_del(tree_node_t* node) {
	if(node == NULL)
		return;
	tstr_free(FSN(node)->name);
	tree_del(node, free);
}
