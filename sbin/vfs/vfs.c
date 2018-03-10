#include <unistd.h>
#include <pmessage.h>
#include <syscall.h>
#include <stdlib.h>
#include <string.h>
#include <kserv/fs.h>
#include <string.h>
#include <vfs.h>
#include <mount.h>
#include <stdio.h>

static TreeNodeT _root;

void doOpen(PackageT* pkg) { 
	const char* name = (const char*)getPackageData(pkg);
	TreeNodeT* node = treeGet(&_root, name);
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
	
	psend(pkg->id, pkg->pid, pkg->type, &fd, 4);
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

void doInfo(PackageT* pkg) { 
	int fd = *(int32_t*)getPackageData(pkg);
	if(fd < 0) {
		psend(pkg->id, pkg->pid, PKG_TYPE_ERR, NULL, 0);
		return;
	}
	
	TreeNodeT* node = (TreeNodeT*)syscall2(SYSCALL_PFILE_NODE, pkg->pid, fd);
	if(node == NULL) {
		psend(pkg->id, pkg->pid, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	FSInfoT info;
	DevTypeT* dev = getDevInfo(node);
	if(dev == NULL || dev->info == NULL) {
		if(node->type == FS_TYPE_FILE) {
			psend(pkg->id, pkg->pid, PKG_TYPE_ERR, NULL, 0);
			return;
		}
		else {
			info.size = sizeof(FSInfoT);
		}
	}
	else {
		dev->info(node, &info);
	}
	info.type = node->type;
	strncpy(info.name, node->name, FNAME_MAX);
	psend(pkg->id, pkg->pid, pkg->type, &info, sizeof(FSInfoT));
}


void doChild(PackageT* pkg) { 
	int fd = *(int32_t*)getPackageData(pkg);
	if(fd < 0) {
		psend(pkg->id, pkg->pid, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	TreeNodeT* node = (TreeNodeT*)syscall2(SYSCALL_PFILE_NODE, pkg->pid, fd);
	if(node == NULL || node->type != FS_TYPE_DIR) {
		psend(pkg->id, pkg->pid, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	int seek = syscall2(SYSCALL_PFILE_GET_SEEK, pkg->pid, fd);
	if(seek == 0) 
		node = node->fChild;
	else 
		node = (TreeNodeT*)seek;

	if(node == NULL || seek == (int)0xFFFFFFFF) {
		psend(pkg->id, pkg->pid, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	FSInfoT info;
	DevTypeT* dev = getDevInfo(node);
	if(dev == NULL || dev->info == NULL) {
		if(node->type == FS_TYPE_FILE) {
			psend(pkg->id, pkg->pid, PKG_TYPE_ERR, NULL, 0);
			return;
		}
	}
	else {
		dev->info(node, &info);
	}

	info.type = node->type;
	if(info.type == FS_TYPE_DIR)
		info.size = sizeof(FSInfoT);

	strncpy(info.name, node->name, FNAME_MAX);
	psend(pkg->id, pkg->pid, pkg->type, &info, sizeof(FSInfoT));

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
		psend(pkg->id, pkg->pid, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	int seek = syscall2(SYSCALL_PFILE_GET_SEEK, pkg->pid, fd);
	if(seek < 0 || size < 0) {
		psend(pkg->id, pkg->pid, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	if(size == 0) {
		psend(pkg->id, pkg->pid, pkg->type, NULL, 0);
		return;
	}

	DevTypeT* dev = getDevInfo(node);
	if(dev != NULL && dev->write != NULL) {
		size = dev->write(node, seek, p, size);
		if(size < 0) {
			psend(pkg->id, pkg->pid, PKG_TYPE_ERR, NULL, 0);
			return;
		}

		psend(pkg->id, pkg->pid, pkg->type, &size, 4);
		seek += size;
		syscall3(SYSCALL_PFILE_SEEK, pkg->pid, fd, seek);
	}
	else {
		psend(pkg->id, pkg->pid, PKG_TYPE_ERR, NULL, 0);
	}
}

void doRead(PackageT* pkg) { 
	const char* p  = (const char*)getPackageData(pkg);
	int fd = *(int32_t*)p;
	int size = *(int32_t*)(p+4);

	TreeNodeT* node = (TreeNodeT*)syscall2(SYSCALL_PFILE_NODE, pkg->pid, fd);
	if(node == NULL) {
		psend(pkg->id, pkg->pid, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	int seek = syscall2(SYSCALL_PFILE_GET_SEEK, pkg->pid, fd);
	if(seek < 0 || size < 0) {
		psend(pkg->id, pkg->pid, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	if(size == 0) {
		psend(pkg->id, pkg->pid, pkg->type, NULL, 0);
		return;
	}

	DevTypeT* dev = getDevInfo(node);
	if(dev != NULL && dev->read != NULL) {
		char* buf = (char*)malloc(size);
		size = dev->read(node, seek, buf, size);
		if(size < 0) {
			psend(pkg->id, pkg->pid, PKG_TYPE_ERR, NULL, 0);
			free(buf);
			return;
		}

		psend(pkg->id, pkg->pid, pkg->type, buf, size);
		free(buf);
		seek += size;
		syscall3(SYSCALL_PFILE_SEEK, pkg->pid, fd, seek);
	}
	else {
		psend(pkg->id, pkg->pid, pkg->type, NULL, 0);
	}
}

void handle(PackageT* pkg) {
	switch(pkg->type) {
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
		case FS_CHILD:
			doChild(pkg);
			break;
	}
}

static void doInit() {
	treeInitNode(&_root);
	strcpy(_root.name, "/");

	TreeNodeT* node = treeSimpleAdd(&_root, "initrd");
	node = mount(node, DEV_SRAMDISK, "initrd");

	node = treeSimpleAdd(&_root, "dev");
	node = treeSimpleAdd(node, "tty0");
	node = mount(node, DEV_TTY, "tty0");
}

void _start() {
	if(fsInited() >= 0) { /*VFS process has been runing .*/
		printf("Panic: 'vfs' process has been running already!\n");
		exit(0);
	}

	doInit();
	syscall1(SYSCALL_KSERV_REG, (int)KSERV_FS_NAME);

	while(true) {
		PackageT* pkg = precv(-1);	
		if(pkg != NULL) {
			handle(pkg);
			free(pkg);
		}
		else
			yield();
	}
	exit(0);
}
