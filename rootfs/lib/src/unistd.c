#include <unistd.h>
#include <vfs/fs.h>
#include <syscall.h>
#include <kstring.h>
#include <stdio.h>
#include <stdlib.h>
#include <procinfo.h>
#include <ext2.h>

int fork() {
	return syscall0(SYSCALL_FORK);
}

int getpid() {
	return syscall0(SYSCALL_GETPID);
}

char* from_sd(const char *filename, int32_t* sz);
static char* read_from_sd(const char* fname, int32_t *size) {
	char* p = from_sd(fname, size);
	return p;
}

static char* read_from_fs(const char* fname, int32_t *size) {
	int fd = open(fname, 0);
	if(fd < 0) 
		return NULL;

	fs_info_t info;
	if(fs_info(fd, &info) != 0 || info.size <= 0) {
		close(fd);
		return NULL;
	}

	char* buf = (char*)malloc(info.size);
	int res = read(fd, buf, info.size);
	close(fd);

	if(res <= 0) {
		free(buf);
		buf = NULL;
		*size = 0;
	}
	*size = info.size;
	return buf;
}

int exec(const char* cmd_line) {
	char* img = NULL;
	int32_t size;
	char cmd[CMD_MAX];
	int i;
	for(i=0; i<CMD_MAX-1; i++) {
		cmd[i] = cmd_line[i];
		if(cmd[i] == 0 || cmd[i] == ' ')
			break;
	}
	cmd[i] = 0;

	if(fs_inited() < 0) {
		img = read_from_sd(cmd, &size);
	}
	else {
		img = read_from_fs(cmd, &size);
	}

	if(img == NULL) {
		printf("'%s' dosn't exist!\n", cmd);
		return -1;
	}
	int res = syscall3(SYSCALL_EXEC_ELF, (int)cmd_line, (int)img, size);
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

static inline unsigned int msleep(unsigned int msecs) {
	return syscall1(SYSCALL_SLEEP_MSEC, msecs);
}

unsigned int usleep(unsigned int usecs) {
	return syscall1(SYSCALL_SLEEP_MSEC, usecs/1000);
}

unsigned int sleep(unsigned int secs) {
	return msleep(secs*1000);
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
