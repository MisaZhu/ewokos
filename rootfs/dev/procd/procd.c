#include <dev/devserv.h>
#include <unistd.h>
#include <stdio.h>
#include <kstring.h>
#include <syscall.h>
#include <vfs/vfs.h>
#include <vfs/fs.h>

static int32_t proc_mount(const char* fname, int32_t index) {
	(void)index;

	vfs_add(fname, "free_mem", 0, NULL);
	vfs_add(fname, "total_mem", 0, NULL);
	vfs_add(fname, "cpu_sec", 0, NULL);
	vfs_add(fname, "cpu_msec", 0, NULL);
	return 0;
}

static int32_t proc_read(int32_t pid, int32_t fd, void* buf, uint32_t size, int32_t seek) {
	(void)pid;
	int32_t ret = -1;
	if(size == 0)
		return 0;

	//fs_info_t info;
	/*if(syscall3(SYSCALL_PFILE_INFO_BY_PID_FD, pid, fd, (int32_t)&info) != 0)
		return ret;
		*/
	tstr_t* fname = vfs_short_name_by_fd(fd);
	if(fname == NULL)
		return ret;

	if(strcmp(CS(fname), "total_mem") == 0) {
		snprintf((char*)buf, size-1, "%d\n", syscall2(SYSCALL_SYSTEM_CMD, 0, 0));
		ret = strlen((char*)buf);
	}
	else if(strcmp(CS(fname), "free_mem") == 0) {
		snprintf((char*)buf, size-1, "%d\n", syscall2(SYSCALL_SYSTEM_CMD, 1, 0));
		ret = strlen((char*)buf);
	}
	else if(strcmp(CS(fname), "cpu_sec") == 0) {
		uint32_t sec;
		syscall3(SYSCALL_SYSTEM_CMD, 4, (int32_t)&sec, 0);
		snprintf((char*)buf, size-1, "%d\n", (int)sec);
		ret = strlen((char*)buf);
	}
	else if(strcmp(CS(fname), "cpu_msec") == 0) {
		uint32_t msec;
		syscall3(SYSCALL_SYSTEM_CMD, 4, 0, (int32_t)&msec);
		snprintf((char*)buf, size-1, "%d\n", (int)msec);
		ret = strlen((char*)buf);
	}
	tstr_free(fname);
	if(seek >= ret)
		ret = 0;
	return ret;
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	device_t dev = {0};
	dev.mount = proc_mount;
	dev.read = proc_read;

	dev_run(&dev, argc, argv);
	return 0;
}
