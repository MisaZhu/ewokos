#include <vfs.h>
#include <kstring.h>

static TreeNodeT _root;

static bool checkAccess(TreeNodeT* node, bool w) {
	return true;
}

void vfsInit() {
	fsTreeNodeInit(&_root);
	strcpy(FSN(&_root)->name, "/");
}

TreeNodeT* vfsAdd(TreeNodeT* nodeTo, FSNodeT* info) {
	if(!checkAccess(nodeTo, true))
		return -1;

	return 0;
}

int32_t vfsDel(TreeNodeT* node) {
	if(!checkAccess(node, true))
		return -1;
	return 0;
}

int32_t vfsInfo(TreeNodeT* node, FSNodeT* info) {
	if(!checkAccess(node, false))
		return -1;
	return 0;
}

TreeNodeT* getNodeByFD(int32_t fd) {
	return NULL;
}

TreeNodeT* getNodeByName(const char* fname) {
	return NULL;
}

TreeNodeT* vfsMount(const char* fname, FSNodeT* info) {
	return NULL;
}

int32_t vfsUnmount(TreeNodeT* node) {
	if(!checkAccess(node, true))
		return -1;
	return 0;
}


