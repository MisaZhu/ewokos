#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ewoksys/syscall.h>
#include <ewoksys/vfs.h>

#include <sysinfo.h>
#include <ewoksys/klog.h>
#include <ewoksys/ipc.h>
#include <ewoksys/proc.h>
#include <ewoksys/wait.h>
#include <dirent.h>
#include <sd/sd.h>
#include <ext2/ext2fs.h>
#include <bsp/bsp_sd.h>

static void* sd_read_ext2(const char* fname, int32_t* size) {
	ext2_t ext2;
	ext2_init(&ext2, sd_read, NULL);
	void* ret = ext2_readfile(&ext2, fname, size);
	ext2_quit(&ext2);
	return ret;
}

static int32_t exec_from_sd(const char* prog) {
	int32_t sz;
	if(bsp_sd_init() != 0){
		printf("bsp init failed\n");
		return -1;
	}

	char* elf = sd_read_ext2(prog, &sz);
	if(elf != NULL) {
		int res = syscall3(SYS_EXEC_ELF, (int32_t)prog, (int32_t)elf, sz);
		free(elf);
		if(res == 0) {
			return res;
		}
	}
	return -1;
}

static void run_before_vfs(const char* cmd) {
	klog("init: %s    ", cmd);

	int pid = fork();
	if(pid == 0) {
		if(exec_from_sd(cmd) != 0) {
			klog("[failed]!\n");
			exit(-1);
		}
	}
	else
		ipc_wait_ready(pid);
	klog("[ok]\n");
}

static void run_init(const char* init_file) {
	if(access(init_file, R_OK) != 0) {
		klog("init: init file '%s' missed! \n", init_file);
		return;
	}

	int pid = fork();
	if(pid == 0) {
		setuid(0);
		setgid(0);
		char cmd[FS_FULL_NAME_MAX];
		snprintf(cmd, FS_FULL_NAME_MAX-1, "/bin/shell -initrd %s", init_file);
		klog("\ninit: loading '%s' ... \n", init_file);
		if(exec(cmd) != 0) {
			klog("[failed]!\n");
			exit(-1);
		}
	}
	else 
		waitpid(pid);
}

static void switch_root(void) {
	run_init("/etc/init.rd");
}

static void halt(void) {
	while(1);
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	if(getuid() >= 0) {
		klog("process 'init' can only loaded by kernel!\n");
		return -1;
	}

	if(getpid() != 0) //not first proc, must be cpu core idle
		halt();
	else
		syscall1(SYS_PROC_SET_CMD, (int32_t)"/sbin/init");

	klog("\n[init process started]\n");
	run_before_vfs("/sbin/core");
	run_before_vfs("/sbin/vfsd");
	run_before_vfs("/sbin/sdfsd");

	switch_root();
	while(true) {
		proc_block_by(getpid(), (uint32_t)main);
	}
	return 0;
}
