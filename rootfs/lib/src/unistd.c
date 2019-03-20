#include <unistd.h>
#include <vfs/fs.h>
#include <syscall.h>
#include <kstring.h>
#include <stdio.h>
#include <stdlib.h>
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
	int fd = fs_open(fname, 0);
	if(fd < 0) 
		return NULL;

	fs_info_t info;
	fsInfo(fd, &info);

	if(info.size <= 0)
		return NULL;

	char* buf = (char*)malloc(info.size);
	int res = fs_read(fd, buf, info.size);
	fs_close(fd);

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
	char cmd[CMD_MAX];
	char fname[NAME_MAX];
	int i;
	for(i=0; i<CMD_MAX-1; i++) {
		cmd[i] = cmdLine[i];
		if(cmd[i] == 0 || cmd[i] == ' ')
			break;
	}
	cmd[i] = 0;

	if(fs_inited() < 0) {
		img = readKernelInitRD(cmd, &size);
	}
	else {
		strcpy(fname, "/initfs/");
		strncpy(fname+ strlen("/initfs/"), cmd, NAME_MAX-strlen("/initfs/")-1);
		img = readFromFS(fname, &size);
	}

	if(img == NULL) {
		printf("'%s' dosn't exist!\n", fname);
		return -1;
	}
	int res = syscall3(SYSCALL_EXEC_ELF, (int)cmdLine, (int)img, size);
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

/*io functions*/

int open(const char* fname, int mode) {
	return fs_open(fname, mode);
}

int write(int fd, const void* buf, uint32_t size) {
	return fs_write(fd, buf, size);
}

int read(int fd, void* buf, uint32_t size) {
	return fs_read(fd, buf, size);
}

void close(int fd) {
	fs_close(fd);
}
