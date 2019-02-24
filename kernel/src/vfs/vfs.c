#include "vfs/vfs.h"
#include "vfs/fstree.h"
#include "mm/kmalloc.h"
#include "kstring.h"
#include "syscalls.h"
#include "proc.h"
#include "kfile.h"
#include "dev/uart.h"

static TreeNodeT _root;
#define MOUNT_MAX 16
static MountT _mounts[MOUNT_MAX];

static inline TreeNodeT* getNodeByFD(int32_t fd) {
	if(fd < 0 || fd >= FILE_MAX)
		return NULL;

	KFileT* kf = _currentProcess->files[fd].kf;
	if(kf == NULL)
		return NULL;
	return (TreeNodeT*)kf->nodeAddr;
}

static inline DeviceT* getNodeDevice(TreeNodeT* node, int32_t *index) {
	int32_t mnt = FSN(node)->mount;
	if(mnt < 0 || mnt >= MOUNT_MAX)
		return NULL;
	if(index != NULL)
		*index = _mounts[mnt].index;
	return _mounts[mnt].device;
}

static TreeNodeT* preMount(DeviceT* device, uint32_t index, bool isFile) {
	TreeNodeT* node = fsNewNode();
	if(node == NULL || device->mount == NULL)
		return NULL;

	int i;
	for(i=0; i<MOUNT_MAX; i++) {
		if(_mounts[i].device == NULL) {
			FSN(node)->mount = i;
			_mounts[i].device = device;
			_mounts[i].servPID = _currentProcess->pid;
			_mounts[i].index = index;
			_mounts[i].to = NULL;
			if(isFile)
				FSN(node)->type = FS_TYPE_FILE;
			FSN(node)->flags |= FS_FLAG_MNT_ROOT;
			device->mount(index, node);
			return node;
		}
	}
	return NULL;
}

static TreeNodeT* mountDevice(TreeNodeT* to, DeviceT* device, uint32_t index, bool isFile) {
	if(to == NULL)
		return NULL;

	if(FSN(to)->mount >= 0) /*can not mount to another mount node */
		return NULL;

	TreeNodeT* node = preMount(device, index, isFile);
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

static TreeNodeT* nodeAdd(TreeNodeT* node, const char* name) {
  if(node == NULL)
    return NULL;
  TreeNodeT* n = fsTreeSimpleGet(node, name);
  if(n != NULL) /*already exist.*/
    return n;

  node = fsTreeSimpleAdd(node, name);
  if(node == NULL)
    return NULL;
  FSN(node)->owner = _currentProcess->owner;

  int32_t index;
  DeviceT* device = getNodeDevice(node, &index);
  if(device != NULL && device->add != NULL) {
    if(!device->add(index, node))
      return NULL;
  }
  return node;
}

static TreeNodeT* nodeAddFull(const char* name) {
  TreeNodeT* node = &_root;

  char n[NAME_MAX+1];
  int j = 0;
  for(int i=0; i<NAME_MAX; i++) {
    n[i] = name[i];
    if(n[i] == 0) {
      return nodeAdd(node, n+j);
    }
    if(n[i] == '/') {
      n[i] = 0;
      node = nodeAdd(node, n+j);
      if(node == NULL)
        return NULL;
      j= i+1;
    }
  }
  return NULL;
}

void vfsInit() {
	for(int i=0; i<MOUNT_MAX; i++) {
		memset(&_mounts[i], 0, sizeof(MountT));
	}

	fsTreeNodeInit(&_root);
	strcpy(FSN(&_root)->name, "/");

	TreeNodeT* node = fsTreeSimpleAdd(&_root, "initrd");
	//node = mountDevice(node, devByName(KDEV_INITRD), 0);	
	node = mountDevice(node, devByName(KDEV_INITRD), 0, false);	
	if(node != NULL) {
		uartPuts("initramdisk mounted.\n");
	}
	fsTreeSimpleAdd(&_root, "dev");
}

static int32_t mountRaw(const char* name, const char* deviceName,  uint32_t index, bool isFile) {
	DeviceT* device = devByName(deviceName);
  TreeNodeT* node = fsTreeGet(&_root, name);
  if(node == NULL) {
    if(name[0] == '/')
      name = name + 1;
    node = nodeAddFull(name);
  }

  if(node == NULL || device == NULL)
    return -1;

  if(mountDevice(node, device, index, isFile) == NULL)
    return -1;
  return 0;
}

int32_t vfsMountFile(const char* name, const char* deviceName,  uint32_t index) {
  return mountRaw(name, deviceName, index, true);
}

int32_t vfsMountDir(const char* name, const char* deviceName,  uint32_t index) {
  return mountRaw(name, deviceName, index, false);
}

int32_t vfsOpen(const char* name, int32_t flags) {
	TreeNodeT* node = fsTreeGet(&_root, name);
	if(node == NULL)
		return -1;

	if(strcmp(name, "/") != 0) {
		int32_t index;
		DeviceT* device = getNodeDevice(node, &index);
		if(device == NULL || device->open == NULL || !device->open(index, node, flags)) {
			if(FSN(node)->type != FS_TYPE_DIR)
				return -1;
		}
	}

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
	TreeNodeT* node = getNodeByFD(fd);
	if(node == NULL)
		return -1;
	int32_t index;
	DeviceT* device = getNodeDevice(node, &index);
	if(device == NULL || device->close == NULL || !device->close(index, node))
		return -1;

	kfUnref(_currentProcess->files[fd].kf, _currentProcess->files[fd].flags);

	_currentProcess->files[fd].kf = NULL;
	_currentProcess->files[fd].flags = 0;
	_currentProcess->files[fd].seek = 0;
	return 0;
}

int32_t vfsRead(int fd, void* buf, uint32_t size, int32_t seek) {
	TreeNodeT* node = getNodeByFD(fd);
	int32_t index;
	DeviceT* device = getNodeDevice(node, &index);
	if(device == NULL || device->read == NULL)
		return -1;
	int32_t sz = device->read(index, node, buf, size, seek);
	if(sz >= 0)
		_currentProcess->files[fd].seek += sz;
	return sz;	
}

int32_t vfsWrite(int fd, void* buf, uint32_t size, int32_t seek) {
	TreeNodeT* node = getNodeByFD(fd);
	int32_t index;
	DeviceT* device = getNodeDevice(node, &index);
	if(device == NULL || device->read == NULL)
		return -1;
	int32_t sz = device->write(index, node, buf, size, seek);
	if(sz >= 0)
		_currentProcess->files[fd].seek += sz;
	return sz;	
}

int vfsNodeInfo(TreeNodeT* node, FSInfoT* info) {
	int32_t index = 0;
	DeviceT* device = getNodeDevice(node, &index);
	if(FSN(node)->type != FS_TYPE_DIR) {
		if(device == NULL || device->info == NULL || !device->info(index, node, info))
			return -1;
	}

	info->id = FSN(node)->id;
	info->type = FSN(node)->type;
	if(info->type == FS_TYPE_DIR)
		info->size = node->size;
	info->owner = FSN(node)->owner;
	strncpy(info->name, FSN(node)->name, NAME_MAX);

	if(device != NULL) {
		strncpy(info->device, device->name, DEV_NAME_MAX);
		info->index = index;
	}
	else {
		info->device[0] = 0;
		info->index = 0;
	}
	return 0;
}

int32_t vfsFInfo(const char* name, FSInfoT* info) {
	TreeNodeT* node = fsTreeGet(&_root, name);
	if(node == NULL)
		return -1;

	return vfsNodeInfo(node, info);
}

int32_t vfsInfo(int32_t fd, FSInfoT* info) {
	TreeNodeT* node = getNodeByFD(fd); 
	if(node == NULL)
		return -1;
	return vfsNodeInfo(node, info);
}

int32_t vfsIoctl(int32_t fd, int32_t cmd, int32_t value) {
	TreeNodeT* node = getNodeByFD(fd); 
	if(node == NULL)
		return -1;
	int32_t index;
	DeviceT* device = getNodeDevice(node, &index);
	if(device == NULL || device->ioctl == NULL || !device->ioctl(index, node, cmd, value))
		return -1;
	return 0;
}

int32_t vfsAdd(int32_t fd, const char* name) {
	TreeNodeT* node = getNodeByFD(fd); 
	if(node == NULL || fsTreeSimpleGet(node, name) != NULL) /*already exist.*/
		return -1;

	node = fsTreeSimpleAdd(node, name);
	if(node == NULL)
		return -1;
	FSN(node)->owner = _currentProcess->owner;

	int32_t index;
	DeviceT* device = getNodeDevice(node, &index);
	if(device == NULL || device->add == NULL || !device->add(index, node))
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
	int32_t index;
	DeviceT* device = getNodeDevice(node, &index);
	if(device == NULL || device->del == NULL || !device->del(index, node))
		return -1;
	treeDel(node, kmfree);
	return 0;
}

static FSInfoT* getNodeKids(TreeNodeT* node, int32_t *num) {
	*num = 0;
	if(node == NULL || node->size == 0)
		return NULL;

	uint32_t size = sizeof(FSInfoT) * node->size;
	FSInfoT* ret = (FSInfoT*)pmalloc(size);
	memset(ret, 0, size);

	uint32_t i;
	TreeNodeT* n = node->fChild;
	for(i=0; i<node->size; i++) {
		if(n == NULL)
			break;

		vfsNodeInfo(n, &ret[i]);
		n = n->next;
	}
	*num = i;
	return ret;
}

FSInfoT* vfsFKids(const char* name, int32_t* num) {
	TreeNodeT* node = fsTreeGet(&_root, name);
	if(node == NULL)
		return NULL;
	return getNodeKids(node, num);
}

FSInfoT* vfsKids(int32_t fd, int32_t* num) {
	TreeNodeT* node = getNodeByFD(fd);
	if(node == NULL)
		return NULL;
	return getNodeKids(node, num);
}

