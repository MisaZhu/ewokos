#include <unistd.h>
#include <vfs/fs.h>
#include <syscall.h>
#include <kstring.h>
#include <malloc.h>
#include <stdio.h>
#include <procinfo.h>

int fork() {
	return syscall0(SYSCALL_FORK);
}

int getpid() {
	return syscall0(SYSCALL_GETPID);
}

static char* readKernelInitRD(const char* fname, int *size) {
	return (char*)syscall3(SYSCALL_INITRD_READ_FILE, (int)fname, 0, (int)size);
}

static char* readFromFS(const char* fname, int *size) {
	int fd = fsOpen(fname, 0);
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

int exec(const char* cmdLine) {
	char* img = NULL;
	int size;
	char cmd[CMD_MAX+1];
	int i;
	for(i=0; i<CMD_MAX; i++) {
		cmd[i] = cmdLine[i];
		if(cmd[i] == 0 || cmd[i] == ' ')
			break;
	}
	cmd[i] = 0;

	if(fsInited() < 0) {
		img = readKernelInitRD(cmd, &size);
	}
	else {
		char fname[128+1];
		strcpy(fname, "/initfs/");
		strncpy(fname+ strlen("/initfs/"), cmd, 128 - strlen("/initfs/"));
		img = readFromFS(fname, &size);
	}
	if(img == NULL)
		return -1;

	int res = syscall2(SYSCALL_EXEC_ELF, (int)cmdLine, (int)img);
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

char* getcwd(char* buf, uint32_t size) {
	return (char*)syscall2(SYSCALL_GET_CWD, (int)buf, (int)size);
}

int chdir(const char* dir) {
	return syscall1(SYSCALL_SET_CWD, (int)dir);
}

int getuid() {
	return syscall1(SYSCALL_GET_UID, getpid());
}

