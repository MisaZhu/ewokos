#include <unistd.h>
#include <vfs/fs.h>
#include <syscall.h>
#include <kstring.h>
#include <tstr.h>
#include <stdio.h>
#include <stdlib.h>
#include <procinfo.h>
#include <ext2.h>

int errno = ENONE;

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
	tstr_t* cmd = tstr_new("", MFS);
	const char *p = cmd_line;
	while(*p != 0 && *p != ' ') {
		tstr_addc(cmd, *p);
		p++;
	}
	tstr_addc(cmd, 0);

	mount_t mnt;
	if(vfs_mount_by_fname("/", &mnt) != 0)
		img = read_from_sd(CS(cmd), &size);
	else 
		img = fs_read_file(CS(cmd), &size);
	if(img == NULL) {
		printf("'%s' dosn't exist!\n", CS(cmd));
		tstr_free(cmd);
		return -1;
	}
	tstr_free(cmd);
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
	fs_info_t info;
	tstr_t* full = fs_full_name(fname);
	const char* full_name = CS(full);
	if(fs_finfo(full_name, &info) != 0) { //not exist.
		if((flags & O_WRONLY) == 0 || (flags & O_CREAT) == 0) {
			tstr_free(full);
			return -1;
		}

		tstr_t *dir = tstr_new("", MFS);
		tstr_t *name = tstr_new("", MFS);
		fs_parse_name(full_name, dir, name);
		int fd = fs_open(CS(dir), O_RDWR);
		int res = -1;
		if(fd >= 0) {
			res = fs_add(fd, CS(name), FS_TYPE_FILE);
			fs_close(fd);
		}
		tstr_free(dir);
		tstr_free(name);
		if(res < 0) {
			tstr_free(full);
			return -1;
		}
	}
	int ret = fs_open(full_name, flags);
	tstr_free(full);
	return ret;
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

int pipe(int fds[2]) {
	return  fs_pipe_open(fds);
}

int dup2(int old_fd, int new_fd) {
	if(old_fd < 0 || new_fd < 0)
		return -1;
	return syscall2(SYSCALL_PFILE_DUP2, old_fd, new_fd);
}
