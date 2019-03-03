#include <vfs.h>
#include <kstring.h>
#include <proc.h>
#include <mm/kmalloc.h>

typedef struct {
	char devName[DEV_NAME_MAX];	
	int32_t devIndex;
	int32_t devServPid;
	TreeNodeT* old;
} MountT;

#define DEV_VFS "dev.vfs"
#define MOUNT_MAX 16

static MountT _mounts[MOUNT_MAX];
static TreeNodeT _root;

static bool checkAccess(TreeNodeT* node, bool w) {
	(void)node;
	(void)w;
	return true;
}

void vfsInit() {
	for(int i=0; i<MOUNT_MAX; i++) {
		memset(&_mounts[i], 0, sizeof(MountT));
	}
	strcpy(_mounts[0].devName, DEV_VFS);

	fsTreeNodeInit(&_root);
	strcpy(FSN(&_root)->name, "/");
}

TreeNodeT* vfsAdd(TreeNodeT* nodeTo, const char* name, uint32_t size) {
	if(!checkAccess(nodeTo, true))
		return NULL;

	TreeNodeT* ret = fsTreeSimpleGet(nodeTo, name);
	if(ret != NULL) 
		return ret;
	
	ret = fsTreeSimpleAdd(nodeTo, name);
	if(size > 0)
		FSN(ret)->type = FS_TYPE_FILE;
	else
		FSN(ret)->type = FS_TYPE_DIR;
	FSN(ret)->size = size;
	FSN(ret)->mount = FSN(nodeTo)->mount;
	FSN(ret)->owner = _currentProcess->owner;
	return ret;
}

int32_t vfsDel(TreeNodeT* node) {
	if(!checkAccess(node, true))
		return -1;
	treeDel(node, kmfree);
	return 0;
}

int32_t vfsNodeInfo(TreeNodeT* node, FSInfoT* info) {
	if(!checkAccess(node, false))
		return -1;
	
	if(FSN(node)->type == FS_TYPE_DIR)
		info->size = node->size;
	else 
		info->size = FSN(node)->size;
	
	info->id = node->id;
	info->node = (uint32_t)node;
	info->type = FSN(node)->type;
	info->owner = FSN(node)->owner;
	strcpy(info->devName, _mounts[FSN(node)->mount].devName);
	info->devIndex = _mounts[FSN(node)->mount].devIndex;
	info->devServPid = _mounts[FSN(node)->mount].devServPid;
	strcpy(info->name, FSN(node)->name);
	return 0;
}

FSInfoT* vfsNodeKids(TreeNodeT* node, int32_t* num) {
	if(!checkAccess(node, false))
		return NULL;

	*num = 0;
	if(node == 0 || node->size == 0)
		return NULL;

	uint32_t size = sizeof(FSInfoT) * node->size;
	FSInfoT* ret = (FSInfoT*)pmalloc(size);
	memset(ret, 0, size);

	uint32_t i;
	*num = node->size;

	TreeNodeT* n = node->fChild;
	for(i=0; i<node->size; i++) {
		if(n == NULL)
			break;
		vfsNodeInfo(n, &ret[i]);
		n = n->next;
	}
	return ret;
}

TreeNodeT* getNodeByFD(int32_t pid, int32_t fd) {
	ProcessT *proc = procGet(pid);
	if(proc == NULL || fd < 0 || fd >= FILE_MAX)
		return NULL;

	KFileT* kf = proc->files[fd].kf;
	if(kf == NULL)
		return NULL;

	return (TreeNodeT*)kf->nodeAddr;
}

TreeNodeT* getNodeByName(const char* fname) {
	return fsTreeGet(&_root, fname);
}

static TreeNodeT* buildNodes(const char* fname) {
	TreeNodeT* father = &_root;
	if(fname[0] == '/')
		fname = fname+1;

	TreeNodeT* node = father;	
	char n[NAME_MAX+1];
	int j = 0;
	for(int i=0; i<NAME_MAX; i++) {
		n[i] = fname[i];
		if(n[i] == 0) {
			return vfsAdd(node, n+j, 0);
		}
		if(n[i] == '/') {
			n[i] = 0; 
			node = vfsAdd(node, n+j, 0);
			if(node == NULL)
				return NULL;
			j= i+1;
		}
	}
	return NULL;
}

TreeNodeT* vfsMount(const char* fname, const char* devName, int32_t devIndex) {
	TreeNodeT* to = buildNodes(fname);
	if(to == NULL)
		return NULL;

	int32_t i;
  for(i=0; i<MOUNT_MAX; i++) {
    if(_mounts[i].devName[0] == 0) {
			strcpy(_mounts[i].devName, devName);
			_mounts[i].devIndex = devIndex;
			_mounts[i].devServPid = _currentProcess->pid;
			_mounts[i].old = to; //save the old node.
			break;
		}
	}
	if(i >= MOUNT_MAX)
		return NULL;

	TreeNodeT* node = fsNewNode();
	if(node == NULL)
		return NULL;

	FSN(node)->owner = _currentProcess->pid;
  FSN(node)->mount = i;
	strcpy(FSN(node)->name, FSN(to)->name);

	/*replace the old node*/
	node->father = to->father;
	node->prev = to->prev;
	if(node->prev != NULL)
		node->prev->next = node;
	if(node->next != NULL)
		node->next->prev = node;
	if(node->father->fChild == to)
		node->father->fChild = node;
	if(node->father->eChild == to)
		node->father->eChild = node;
	return node;
}

int32_t vfsUnmount(TreeNodeT* node) {
	if(!checkAccess(node, true))
		return -1;
	return 0;
}

