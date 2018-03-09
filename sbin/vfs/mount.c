#include <tree.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <mount.h>
#include <dev/dev_sramdisk.h>

#define MOUNT_MAX 16

static MountT _mounts[MOUNT_MAX];

static DevTypeT _devices[] = {
	[DEV_NONE] = { 0 },
	[DEV_SRAMDISK] = { mountSRamDisk, 0, readSRamDisk, 0, infoSRamDisk, 0},
};
	
void mountInit() {
	for(int i=0; i<MOUNT_MAX; i++) {
		memset(&_mounts[i], 0, sizeof(MountT));
	}
}

static void mountDevice(TreeNodeT* node) {
	printf("mnt: type=%d, device=%s\n",
		_mounts[node->mount].type,
		_mounts[node->mount].device);
	
	DevTypeT* dev = getDevInfo(node);
	if(dev != NULL && dev->mount != NULL)
		dev->mount(node);
}

static TreeNodeT* preMount(uint32_t type, const char* device) {
	TreeNodeT* node = (TreeNodeT*)malloc(sizeof(TreeNodeT));
	if(node == NULL)
		return NULL;
	treeInitNode(node);

	int i;
	for(i=0; i<MOUNT_MAX; i++) {
		if(_mounts[i].device[0] == 0) {
			node->mount = i;
			node->type = type;

			strncpy(_mounts[i].device, device, NAME_MAX);
			_mounts[i].type = type;
			_mounts[i].to = NULL;
			mountDevice(node);
			node->flags |= FS_FLAG_MNT_ROOT;
			return node;
		}
	}
	return NULL;
}

TreeNodeT* mount(TreeNodeT* to, uint32_t type, const char* device) {
	if(to == NULL || device == NULL)
		return NULL;

	if(to->mount >= 0) /*can not mount to another mount node */
		return NULL;

	TreeNodeT* node = preMount(type, device);
	if(node == NULL)
		return NULL;
	strcpy(node->name, to->name);

	_mounts[node->mount].to = to; //save the old node.

	/*replace the old node*/
	node->father = to->father;
	node->prev = to->prev;
	if(node->prev != NULL)
		node->prev->next = node;

	node->next = to->next;
	if(node->next != NULL)
		node->next->prev = node;

	if(node->father->fChild == to)
		node->father->fChild = node;
	if(node->father->eChild == to)
		node->father->eChild = node;
	
	return node;
}

void unmount(TreeNodeT* node) {
	if(node == NULL)
		return;

	TreeNodeT* to = _mounts[node->mount].to; //get the saved old node.
	if(to == NULL)
		return;

	to->father = node->father;
	to->prev = node->prev;
	if(to->prev != NULL)
		to->prev->next = to;

	to->next = node->next;
	if(to->next != NULL)
		to->next->prev = to;

	if(to->father->fChild == node) 
		to->father->fChild = to;
	if(to->father->eChild == node)
		to->father->eChild = to;
	
	treeDel(node);	
}

MountT* getMountInfo(struct TreeNode* node) {
	if(node == NULL || node->mount < 0)
		return NULL;

	return &_mounts[node->mount];
}

DevTypeT* getDevInfo(struct TreeNode* node) {
	if(node == NULL || node->mount < 0)
		return NULL;

	return &_devices[_mounts[node->mount].type];
}
