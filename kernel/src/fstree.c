#include <fstree.h>
#include <kstring.h>
#include <fsinfo.h>
#include <mm/kmalloc.h>

static uint32_t _nodeIDCounter = 0;

void fsTreeNodeInit(TreeNodeT* node) {
	treeInitNode(node);
	node->id = _nodeIDCounter++;
	node->data = kmalloc(sizeof(FSNodeT));
	FSNodeT* fn = (FSNodeT*)node->data;
	fn->name[0] = 0;
	fn->mount = 0;
	fn->type = 0;
	fn->owner = 0;
}

TreeNodeT* fsNewNode() {
	TreeNodeT* ret = (TreeNodeT*)kmalloc(sizeof(TreeNodeT));
	fsTreeNodeInit(ret);
	return ret;
}

TreeNodeT* fsTreeSimpleGet(TreeNodeT* father, const char* name) {
	if(father == NULL || strchr(name, '/') != NULL)
		return NULL;

	TreeNodeT* node = father->fChild;
	while(node != NULL) {
		const char* n = FSN(node)->name;
		if(strcmp(n, name) == 0) {
			return node;
		}
		node = node->next;
	}
	return NULL;
}

TreeNodeT* fsTreeGet(TreeNodeT* father, const char* name) {
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
	
TreeNodeT* fsTreeSimpleAdd(TreeNodeT* father, const char* name) {
	TreeNodeT* node = fsNewNode();
	FSNodeT* data = FSN(node);
	if(node == NULL ||
			data->type != FS_TYPE_DIR ||
			strchr(name, '/') != NULL)
		return NULL;

	strncpy(data->name, name, NAME_MAX);
	treeAdd(father, node);
	return node;
}

