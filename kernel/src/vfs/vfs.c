#include "vfs/vfs.h"
#include "vfs/fstree.h"
#include "mm/kmalloc.h"
#include "kstring.h"
#include "syscalls.h"
#include "proc.h"
#include "kfile.h"

static TreeNodeT _root;
#define MOUNT_MAX 16
static MountT _mounts[MOUNT_MAX];

static inline TreeNodeT* getNodeByFD(int32_t fd) {
  KFileT* kf = _currentProcess->files[fd].kf;
  if(kf == NULL)
    return NULL;
	return (TreeNodeT*)kf->nodeAddr;
}

static inline DeviceT* getNodeDevice(TreeNodeT* node) {
	int32_t mnt = FSN(node)->mount;
	if(mnt < 0 || mnt >= MOUNT_MAX)
		return NULL;
	return _mounts[mnt].device;
}

static TreeNodeT* preMount(DeviceT* device, uint32_t index) {
  TreeNodeT* node = fsNewNode();
  if(node == NULL)
    return NULL;

  int i;
  for(i=0; i<MOUNT_MAX; i++) {
    if(_mounts[i].device == NULL) {
      FSN(node)->mount = i;
      _mounts[i].device = device;
      _mounts[i].index = index;
      _mounts[i].to = NULL;
			device->mount(node);
      FSN(node)->flags |= FS_FLAG_MNT_ROOT;
      return node;
    }
  }
  return NULL;
}

static TreeNodeT* mountDevice(TreeNodeT* to, DeviceT* device, uint32_t index) {
	if(to == NULL)
    return NULL;

  if(FSN(to)->mount >= 0) /*can not mount to another mount node */
    return NULL;

  TreeNodeT* node = preMount(device, index);
  if(node == NULL)
    return NULL;
  strcpy(FSN(node)->name, FSN(to)->name);

  _mounts[FSN(node)->mount].to = to; //save the old node.

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

void vfsInit() {
  for(int i=0; i<MOUNT_MAX; i++) {
    memset(&_mounts[i], 0, sizeof(MountT));
  }

	fsTreeNodeInit(&_root);
  strcpy(FSN(&_root)->name, "/");

	TreeNodeT* node = fsTreeSimpleAdd(&_root, "initrd");
  //node = mountDevice(node, devByName(KDEV_INITRD), 0);	
  node = mountDevice(node, devByName(KDEV_INITRD), 0);	
}

int32_t vfsMount(const char* name, const char* deviceName, uint32_t index) {
	TreeNodeT* node = fsTreeGet(&_root, name);
	DeviceT* device = devByName(deviceName);	
  if(node == NULL || device == NULL)
		return -1;

	if(mountDevice(node, device, index) == NULL)
		return -1;
	return 0;
}

int32_t vfsOpen(const char* name, int32_t flags) {
  TreeNodeT* node = fsTreeGet(&_root, name);
  if(node == NULL)
		return -1;
	
	DeviceT* device = getNodeDevice(node);
	if(device == NULL || device->open == NULL || !device->open(node, flags))
		return -1;

  KFileT* kf = kfOpen((uint32_t)node);
  if(kf == NULL)
    return -1;

  kfRef(kf, flags);
  int32_t i;
  for(i=0; i<FILE_MAX; i++) {
    if(_currentProcess->files[i].kf == NULL) {
      _currentProcess->files[i].kf = kf;
      _currentProcess->files[i].flags = flags;
      _currentProcess->files[i].seek = 0;
			return i;
    }
  }
	kfUnref(kf, flags);
  return -1;
}

int32_t vfsClose(int32_t fd) {
  if(fd < 0 || fd >= FILE_MAX)
    return -1;

  TreeNodeT* node = getNodeByFD(fd);
  if(node == NULL)
		return -1;

	DeviceT* device = getNodeDevice(node);
	if(device == NULL || device->close == NULL || !device->close(node))
		return -1;

  kfUnref(_currentProcess->files[fd].kf, _currentProcess->files[fd].flags);

  _currentProcess->files[fd].kf = NULL;
  _currentProcess->files[fd].flags = 0;
  _currentProcess->files[fd].seek = 0;
  return 0;
}

int32_t vfsFInfo(const char* name, FSInfoT* info) {
 	TreeNodeT* node = fsTreeGet(&_root, name);
	if(node == NULL)
		return -1;

	DeviceT* device = getNodeDevice(node);
	if(device == NULL || device->info == NULL || !device->info(node, info))
		return -1;
	return 0;
}

int32_t vfsInfo(int32_t fd, FSInfoT* info) {
 	TreeNodeT* node = getNodeByFD(fd); 
	if(node == NULL)
		return -1;

	DeviceT* device = getNodeDevice(node);
	if(device == NULL || device->info == NULL || !device->info(node, info))
		return -1;
	return 0;
}

int32_t vfsSetopt(int32_t fd, int32_t cmd, int32_t value) {
 	TreeNodeT* node = getNodeByFD(fd); 
	if(node == NULL)
		return -1;

	DeviceT* device = getNodeDevice(node);
	if(device == NULL || device->setopt == NULL || !device->setopt(node, cmd, value))
		return -1;
	return 0;
}

int32_t vfsAdd(int32_t fd, const char* name) {
  if(fd < 0 || fd >= FILE_MAX)
    return -1;

 	TreeNodeT* node = getNodeByFD(fd); 
	if(node == NULL || fsTreeSimpleGet(node, name) != NULL) /*already exist.*/
    return -1;
 
  node = fsTreeSimpleAdd(node, name);
	if(node == NULL)
		return -1;
	FSN(node)->owner = _currentProcess->owner;

	DeviceT* device = getNodeDevice(node);
	if(device == NULL || device->add == NULL || !device->add(node))
		return -1;
	return 0;
}

int32_t vfsDel(int32_t fd, const char* name) {
 	TreeNodeT* node = getNodeByFD(fd); 
	if(node == NULL)
		return -1;
	
	node = fsTreeGet(node, name);
	if(node == NULL)
		return -1;

	DeviceT* device = getNodeDevice(node);
	if(device == NULL || device->del == NULL || !device->del(node))
		return -1;
	treeDel(node, kmfree);
	return 0;
}

