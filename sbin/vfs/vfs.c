#include <types.h>
#include <fork.h>
#include <pmessage.h>
#include <syscall.h>
#include <malloc.h>
#include <string.h>
#include <sramdisk.h>
#include <stdio.h>
#include <kserv/kserv.h>
#include <kserv/fs.h>
#include <string.h>
#include <vfs.h>
#include <mount.h>

static TreeNodeT _root;

void doOpen(PackageT* pkg) { 
	const char* name = (const char*)getPackageData(pkg);
	TreeNodeT* node = treeGet(&_root, name);
	int32_t fd = -1;
	int openMode = 0;//read.

	if(node != NULL) {
		printf("nodeName: %s\n", node->name);
		DevTypeT* dev = getDevInfo(node);
		if(dev != NULL && dev->open != NULL) {
			if(!dev->open(node)) {//open failed;
				free(node);
				node = NULL;
			}
		}
		fd = syscall2(SYSCALL_PFILE_OPEN, (int32_t)node, openMode);
	}
	
	psend(pkg->id, pkg->pid, pkg->type, &fd, 4);
}

void doClose(PackageT* pkg) { 
	int fd = *(int32_t*)getPackageData(pkg);
	if(fd < 0)
		return;
	
	TreeNodeT* node = (TreeNodeT*)syscall1(SYSCALL_PFILE_NODE, fd);
	if(node == NULL)
		return;

	DevTypeT* dev = getDevInfo(node);
	if(dev != NULL && dev->close != NULL) {
		dev->close(node);
	}

	syscall1(SYSCALL_PFILE_CLOSE, fd);
}

void doWrite(PackageT* pkg) { 
	(void)pkg;
}

void doRead(PackageT* pkg) { 
	(void)pkg;
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
			doClose(pkg);
			break;
	}
}

static void doInit() {
	treeInitNode(&_root);
	strcpy(_root.name, "/");

	TreeNodeT* node = treeSimpleAdd(&_root, "initrd");
	node = mount(node, DEV_SRAMDISK, "initrd");
}

void _start() {
	psend(-1, 0, KS_REG, (void*)KSERV_FS_NAME, strlen(KSERV_FS_NAME)+1);
	doInit();
	fsInit(getpid());

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
