#include <fork.h>
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

static int execFile(const char* fname) {
	int fd = fsOpen(fname);
	if(fd < 0) 
		return -1;

	FSInfoT info;
	fsInfo(fd, &info);

	if(info.size <= 0)
		return -1;

	char* buf = (char*)malloc(info.size);
	int res = fsRead(fd, buf, info.size);
	fsClose(fd);
	if(res > 0)
		res = syscall1(SYSCALL_EXEC_ELF, (int)buf);
	else 
		res = -1;
	free(buf);
	return res;
}

int exec(const char* cmd) {
	if(fsInited() < 0) {
		return syscall1(SYSCALL_EXEC, (int)cmd);
	}

	/*load img from exec path*/
	char fname[128+1];
	strcpy(fname, "/initrd/");
	strncpy(fname+ strlen("/initrd/"), cmd, 128 - strlen("/initrd/"));
	execFile(fname);
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
