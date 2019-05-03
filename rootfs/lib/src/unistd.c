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

	mount_t mnt;
	if(vfs_mount_by_fname("/", &mnt) != 0)
		img = read_from_sd(cmd, &size);
	else 
		img = fs_read_file(cmd, &size);
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

char* getcwd(char* buf, uint32_t size) {
	return (char*)syscall2(SYSCALL_GET_CWD, (int)buf, (int)size-1);
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

int open(const char* fname, int flags) {
	if(fname[0] == 0)
		return -1;
	char full[FULL_NAME_MAX];
	fs_full_name(fname, full, FULL_NAME_MAX);

	fs_info_t info;
	if(fs_finfo(full, &info) != 0) { //not exist.
		if((flags & O_WRONLY) == 0 ||
				(flags & O_CREAT) == 0)
			return -1;

		char dir[FULL_NAME_MAX];
		char name[SHORT_NAME_MAX];
		fs_parse_name(full, dir, FULL_NAME_MAX, name, SHORT_NAME_MAX);	

		int fd = fs_open(dir, O_RDWR);
		if(fd < 0)
			return -1;
		int res = fs_add(fd, name, FS_TYPE_FILE);
		fs_close(fd);
		if(res < 0)
			return -1;
	}
	return fs_open(full, flags);
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
