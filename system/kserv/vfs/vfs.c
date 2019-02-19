#include <unistd.h>
#include <pmessage.h>
#include <syscall.h>
#include <stdlib.h>
#include <kstring.h>
#include <kserv/kserv.h>
#include <kserv/fs.h>
#include <vfs.h>
#include <mount.h>
#include <stdio.h>
#include <proto.h>

static TreeNodeT _root;

void doOpen(PackageT* pkg) { 
	const char* name = (const char*)getPackageData(pkg);
	TreeNodeT* node = fsTreeGet(&_root, name);
	int32_t fd = -1;
	int openMode = 0;//read.

	if(node != NULL) {
		DevTypeT* dev = getDevInfo(node);
		if(dev != NULL && dev->open != NULL) {
			if(!dev->open(node)) {//open failed;
				free(node);
				node = NULL;
			}
		}
		fd = syscall3(SYSCALL_PFILE_OPEN, pkg->pid, (int32_t)node, openMode);
	}
	
	psend(pkg->id, pkg->type, &fd, 4);
}

void doClose(PackageT* pkg) { 
	int fd = *(int32_t*)getPackageData(pkg);
	if(fd < 0)
		return;
	
	TreeNodeT* node = (TreeNodeT*)syscall2(SYSCALL_PFILE_NODE, pkg->pid, fd);
	if(node == NULL)
		return;

	DevTypeT* dev = getDevInfo(node);
	if(dev != NULL && dev->close != NULL) {
		dev->close(node);
	}

	syscall1(SYSCALL_PFILE_CLOSE, fd);
}

void doFDev(PackageT* pkg) { 
	const char* name = (const char*)getPackageData(pkg);
	TreeNodeT* node = fsTreeGet(&_root, name);
	
	if(node == NULL) {
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	MountT* mnt = getMountInfo(node);
	if(mnt == NULL) {
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}
	psend(pkg->id, pkg->type, mnt->device, strlen(mnt->device)+1);
}

void doDev(PackageT* pkg) { 
	int fd = *(int32_t*)getPackageData(pkg);
	if(fd < 0) {
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}
	
	TreeNodeT* node = (TreeNodeT*)syscall2(SYSCALL_PFILE_NODE, pkg->pid, fd);
	if(node == NULL) {
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	MountT* mnt = getMountInfo(node);
	if(mnt == NULL) {
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}
	psend(pkg->id, pkg->type, mnt->device, strlen(mnt->device)+1);
}

static bool getNodeInfo(TreeNodeT* node, FSInfoT* info) {
	DevTypeT* dev = getDevInfo(node);
	if(dev == NULL || dev->info == NULL) {
		if(FSN(node)->type == FS_TYPE_FILE) {
			return false;
		}
		else {
			info->size = sizeof(FSInfoT);
		}
	}
	else {
		dev->info(node, info);
	}

	info->id = FSN(node)->id;
	info->type = FSN(node)->type;
	if(info->type == FS_TYPE_DIR)
		info->size = sizeof(FSInfoT);
	info->owner = FSN(node)->owner;
	strncpy(info->name, FSN(node)->name, NAME_MAX);

	MountT* mnt = getMountInfo(node);
	if(mnt != NULL) {
		strncpy(info->device, mnt->device, DEV_NAME_MAX);
		info->index = mnt->index;
	}
	else {
		info->device[0] = 0;
		info->index = 0;
	}
	return true;
}

void doFInfo(PackageT* pkg) { 
	const char* name = (const char*)getPackageData(pkg);
	TreeNodeT* node = fsTreeGet(&_root, name);
	
	FSInfoT info;
	if(node == NULL || !getNodeInfo(node, &info)) {
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	psend(pkg->id, pkg->type, &info, sizeof(FSInfoT));
}

void doInfo(PackageT* pkg) { 
	int fd = *(int32_t*)getPackageData(pkg);
	if(fd < 0) {
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}
	
	FSInfoT info;
	TreeNodeT* node = (TreeNodeT*)syscall2(SYSCALL_PFILE_NODE, pkg->pid, fd);
	if(node == NULL || !getNodeInfo(node, &info)) {
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}
	psend(pkg->id, pkg->type, &info, sizeof(FSInfoT));
}

void doChild(PackageT* pkg) { 
	int fd = *(int32_t*)getPackageData(pkg);
	if(fd < 0) {
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	TreeNodeT* node = (TreeNodeT*)syscall2(SYSCALL_PFILE_NODE, pkg->pid, fd);
	if(node == NULL || FSN(node)->type != FS_TYPE_DIR) {
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	int seek = syscall2(SYSCALL_PFILE_GET_SEEK, pkg->pid, fd);
	if(seek == 0) 
		node = node->fChild;
	else 
		node = (TreeNodeT*)seek;

	if(node == NULL || seek == (int)0xFFFFFFFF) {
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	FSInfoT info;
	if(!getNodeInfo(node, &info)) {
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	psend(pkg->id, pkg->type, &info, sizeof(FSInfoT));

	if(node->next == NULL) 
		seek = 0xFFFFFFFF;
	else
		seek = (int)node->next;
	syscall3(SYSCALL_PFILE_SEEK, pkg->pid, fd, seek);
}

void doWrite(PackageT* pkg) { 
	const char* p  = (const char*)getPackageData(pkg);
	int fd = *(int32_t*)p;
	int size = *(int32_t*)(p+4);
	p = p + 8;

	TreeNodeT* node = (TreeNodeT*)syscall2(SYSCALL_PFILE_NODE, pkg->pid, fd);
	if(node == NULL) {
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	int seek = syscall2(SYSCALL_PFILE_GET_SEEK, pkg->pid, fd);
	if(seek < 0 || size < 0) {
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	if(size == 0) {
		psend(pkg->id, pkg->type, NULL, 0);
		return;
	}

	DevTypeT* dev = getDevInfo(node);
	if(dev != NULL && dev->write != NULL) {
		size = dev->write(node, seek, p, size);
		if(size < 0) {
			psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
			return;
		}

		psend(pkg->id, pkg->type, &size, 4);
		seek += size;
		syscall3(SYSCALL_PFILE_SEEK, pkg->pid, fd, seek);
	}
	else {
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
	}
}

void doAdd(PackageT* pkg) { 
	const char* p  = (const char*)getPackageData(pkg);
	int fd = *(int32_t*)p;
	int size = *(int32_t*)(p+4);
	p = p + 8;

	TreeNodeT* node = (TreeNodeT*)syscall2(SYSCALL_PFILE_NODE, pkg->pid, fd);
	if(node == NULL || size <= 0) {
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	if(fsTreeSimpleGet(node, p) != NULL) { /*already exist.*/
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	node = fsTreeSimpleAdd(node, p);
	if(node == NULL) {
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	FSN(node)->owner = syscall1(SYSCALL_GET_UID, pkg->pid);
	psend(pkg->id, pkg->type, NULL, 0);
}

void doRead(PackageT* pkg) { 
	ProtoT proto;
	protoInit(&proto, getPackageData(pkg), pkg->size);
	int fd = protoReadInt(&proto);
	int size = protoReadInt(&proto);

	TreeNodeT* node = (TreeNodeT*)syscall2(SYSCALL_PFILE_NODE, pkg->pid, fd);
	if(node == NULL) {
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	int seek = syscall2(SYSCALL_PFILE_GET_SEEK, pkg->pid, fd);
	if(seek < 0 || size < 0) {
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	if(size == 0) {
		psend(pkg->id, pkg->type, NULL, 0);
		return;
	}

	DevTypeT* dev = getDevInfo(node);
	if(dev != NULL && dev->read != NULL) {
		char* buf = (char*)malloc(size);
		size = dev->read(node, seek, buf, size);
		if(size < 0) {
			psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
			free(buf);
			return;
		}

		psend(pkg->id, pkg->type, buf, size);
		free(buf);
		seek += size;
		syscall3(SYSCALL_PFILE_SEEK, pkg->pid, fd, seek);
	}
	else {
		psend(pkg->id, pkg->type, NULL, 0);
	}
}

void doMount(PackageT* pkg) {
	ProtoT proto;
	protoInit(&proto, getPackageData(pkg), pkg->size);
	const char* device = protoReadStr(&proto);
	uint32_t index = protoReadInt(&proto);
	const char* path = protoReadStr(&proto);
	const char* name = protoReadStr(&proto);

	TreeNodeT* node = fsTreeGet(&_root, path);
	if(node == NULL) { 
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	node = fsTreeSimpleAdd(node, name);
	node = mount1(node, DEV_TTY, device, index);
	psend(pkg->id, pkg->type, NULL, 0);
}

void handle(PackageT* pkg, void* p) {
	(void)p;
	switch(pkg->type) {
		case FS_FDEV:
			doFDev(pkg);
			break;
		case FS_DEV:
			doDev(pkg);
			break;
		case FS_OPEN:
			doOpen(pkg);
			break;
		case FS_CLOSE:
			doClose(pkg);
			break;
		case FS_WRITE:
			doWrite(pkg);
			break;
		case FS_READ:
			doRead(pkg);
			break;
		case FS_INFO:
			doInfo(pkg);
			break;
		case FS_FINFO:
			doFInfo(pkg);
			break;
		case FS_CHILD:
			doChild(pkg);
			break;
		case FS_ADD:
			doAdd(pkg);
			break;
		case FS_MOUNT:
			doMount(pkg);
			break;
	}
}

static void doInit() {
	_fsNodeIndex = 0;
	fsTreeNodeInit(&_root);
	strcpy(FSN(&_root)->name, "/");

	TreeNodeT* node = fsTreeSimpleAdd(&_root, "initrd");
	node = mount1(node, DEV_SRAMDISK, DEV_VFS, 0);

	node = fsTreeSimpleAdd(&_root, "dev");
	node = fsTreeSimpleAdd(node, "tty0");
	node = mount1(node, DEV_TTY, "tty", 0);
}

void _start() {
	if(fsInited() >= 0) { /*VFS process has been runing .*/
		printf("Panic: 'vfs' process has been running already!\n");
		exit(0);
	}

	doInit();

	if(!kservRun(KSERV_VFS_NAME, handle, NULL))
		exit(0);
}
