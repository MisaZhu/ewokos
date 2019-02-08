#include <string.h>
#include <vfs.h>
#include <tree.h>
#include <syscall.h>
#include <malloc.h>
#include <stdio.h>
#include <kserv/fs.h>

/*need to be freed later*/
static char* readKernelInitRD(const char* fname, int seek, int *size) {
	return (char*)syscall3(SYSCALL_INITRD_READ, (int)fname, seek, (int)size);
}

static int infoKernelInitRD(const char* fname, FSInfoT* info) {
	return syscall2(SYSCALL_INITRD_INFO, (int)fname, (int)info);
}

/*need to be freed later,
one filename each line.
*/

static char* getKernelInitRDFiles() {
	return (char*)syscall0(SYSCALL_INITRD_FILES);
}

void mountSRamDisk(TreeNodeT* node) {
	node->type = FS_TYPE_DIR;
	char* s = getKernelInitRDFiles();
	if(s == NULL)
		return;
	
	TreeNodeT* n;
	int i = 0;
	char* name = s;
	while(1) {
		if(s[i] == 0) {
			if(name[0] != 0) {
				n = treeSimpleAdd(node, name);
				if(n != NULL) {
					n->type = FS_TYPE_FILE;
					n->mount = node->mount;
				}
			}
			break;
		}
		else if(s[i] == '\n') {
			s[i] = 0;
			if(name[0] != 0) {
				n = treeSimpleAdd(node, name);
				if(n != NULL) {
					n->type = FS_TYPE_FILE;
					n->mount = node->mount;
				}
			}
			name = s + i + 1;
		}
		i++;
	}
	free(s);
	node->type = FS_TYPE_DIR;
}

int readSRamDisk(TreeNodeT* node, int seek, char* buf, uint32_t size) {
	int sz = size;
	char* p = readKernelInitRD(node->name, seek, &sz);
	if(p == NULL)
		return -1;

	memcpy(buf, p, sz);
	free(p);
	return sz;
}

int infoSRamDisk(TreeNodeT* node, FSInfoT* info) {
	return infoKernelInitRD(node->name, info);
}
