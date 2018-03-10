#include <unistd.h>
#include <kserv/fs.h>
#include <syscall.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>

int fork() {
	return syscall0(SYSCALL_FORK);
}

int getpid() {
	return syscall0(SYSCALL_GETPID);
}

static char* readKernelInitRD(const char* fname, int *size) {
	return (char*)syscall3(SYSCALL_INITRD_READ, (int)fname, 0, (int)size);
}

static char* readFromFS(const char* fname, int *size) {
	int fd = fsOpen(fname);
	if(fd < 0) 
		return NULL;

	FSInfoT info;
	fsInfo(fd, &info);

	if(info.size <= 0)
		return NULL;

	char* buf = (char*)malloc(info.size);
	int res = fsRead(fd, buf, info.size);
	fsClose(fd);

	if(res <= 0) {
		free(buf);
		*size = 0;
	}

	*size = info.size;
	return buf;
}

int exec(const char* cmd) {
	char* img = NULL;
	int size;

	if(fsInited() < 0) {
		img = readKernelInitRD(cmd, &size);
	}
	else {
		/*load img from exec path*/
		char fname[128+1];
		strcpy(fname, "/initrd/");
		strncpy(fname+ strlen("/initrd/"), cmd, 128 - strlen("/initrd/"));
		img = readFromFS(fname, &size);
	}
	if(img == NULL)
		return -1;

	int res = syscall2(SYSCALL_EXEC_ELF, (int)cmd, (int)img);
	free(img);
	return res;
}

void exit(int code) {
	syscall1(SYSCALL_EXIT, code);
}

void wait(int pid) {
	syscall1(SYSCALL_WAIT, pid);
}

void yield() {
	syscall0(SYSCALL_YIELD);
}

char* getcwd(char* buf, size_t size) {
	return (char*)syscall2(SYSCALL_GET_CWD, (int)buf, (int)size);
}

int chdir(const char* dir) {
	return syscall1(SYSCALL_SET_CWD, (int)dir);
}
