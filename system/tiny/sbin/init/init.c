#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/vfs.h>
#include <vprintf.h>
#include <sysinfo.h>
#include <sys/klog.h>
#include <sys/ipc.h>
#include <sys/proc.h>

static char* run_none_fs_kfs(const char* cmd, int32_t *size) {
	int32_t index = 0;
	char* data = NULL;
	while(true) {
		char name[FS_FULL_NAME_MAX];
		int32_t sz = syscall3(SYS_KROMFS_GET, index, (int32_t)name, (int32_t)NULL);	
		if(sz < 0) 
			break;
		if(sz > 0 && name[0] != 0 && strcmp(cmd, name) == 0)  {
			data = (char*)malloc(sz);
			sz = syscall3(SYS_KROMFS_GET, index, (int32_t)name, (int32_t)data);	
			*size = sz;
			return data;
		}
		index++;
	}
	return NULL;	
}

static int run_none_fs(const char* cmd) {
	klog("init: %s ", cmd);
	int pid = fork();
	if(pid == 0) {
		char* data = NULL;
		int32_t sz = 0;
		data = run_none_fs_kfs(cmd, &sz);
		if(data == NULL) {
			klog("[error!] (%s)\n", cmd);
			exit(-1);
		}
		proc_exec_elf(cmd, data, sz);
		free(data);
	}
	proc_wait_ready(pid);
	klog("[ok]\n");
	return pid;
}

static int run(const char* cmd, bool prompt, bool wait) {
	if(prompt)
		klog("init: %s ", cmd);

	int pid = fork();
	if(pid == 0) {
		proc_detach();
		if(exec(cmd) != 0) {
			if(prompt)
				klog("[error!]\n");
			exit(-1);
		}
	}
	else if(wait)
		proc_wait_ready(pid);

	if(prompt)
		klog("[ok]\n");
	return 0;
}

static const char* read_line(int fd) {
	static char line[256];
	int i = 0;
	while(i < 255) {
		char c;
		if(read(fd, &c, 1) <= 0) {
			if(i == 0)
				return NULL;
			else
				break;
		}	

		if(c == '\n')
			break;
		line[i] = c;
		i++;
	}
	line[i] = 0;
	return line;
}

static void load_devs(void) {
	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (int32_t)&sysinfo);
	char fn[FS_FULL_NAME_MAX];
	snprintf(fn, FS_FULL_NAME_MAX-1, "/etc/arch/%s/init.dev", sysinfo.machine);
	int fd = open(fn, O_RDONLY);
	if(fd < 0)
		return;

	while(true) {
		const char* ln = read_line(fd);
		if(ln == NULL)
			break;
		if(ln[0] == 0 || ln[0] == '#')
			continue;
		run(ln, true, true);
	}
	close(fd);
}

static void run_procs(void) {
	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (int32_t)&sysinfo);
	char fn[FS_FULL_NAME_MAX];
	snprintf(fn, FS_FULL_NAME_MAX-1, "/etc/arch/%s/init.rd", sysinfo.machine);
	
	int fd = open(fn, O_RDONLY);
	if(fd < 0)
		return;

	while(true) {
		const char* ln = read_line(fd);
		if(ln == NULL)
			break;
		if(ln[0] == 0 || ln[0] == '#')
			continue;
		run(ln, false, false);
	}
	close(fd);
}

void core(void);
static void run_core(void) {
	int pid = fork();
	if(pid == 0) {
		syscall1(SYS_PROC_SET_CMD, (int32_t)"init-core");
		core();
	}
	else
		proc_wait_ready(pid);
}

int vfsd_main(void);
static void run_vfsd(void) {
	int pid = fork();
	if(pid == 0) {
		syscall1(SYS_PROC_SET_CMD, (int32_t)"init-vfsd");
		vfsd_main();
	}
	else
		proc_wait_ready(pid);
}

int procd_main(void);
static void run_procd(void) {
	int pid = fork();
	if(pid == 0) {
		syscall1(SYS_PROC_SET_CMD, (int32_t)"init-procd");
		procd_main();
	}
	else
		proc_wait_ready(pid);
}

static void init_fs(void) {
	run_none_fs("/drivers/rootfsd");
}

static void init_tty_stdio(void) {
	int fd = open("/dev/tty0", 0);

	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);
}

static void load_stdios(void) {
	run("/drivers/stdiod stdin", false, false);
	run("/drivers/stdiod stdout", false, false);
	run("/drivers/stdiod stderr", false, false);
}

static void switch_root(void) {
	int pid = fork();
	if(pid == 0) {
		setuid(0);
		load_devs();
		init_tty_stdio();
		load_stdios();
		run_procs();
		exit(0);
	}
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	if(getuid() >= 0) {
		klog("process 'init' can only loaded by kernel!\n");
		return -1;
	}

	klog("\n[init process started]\n");
	syscall1(SYS_PROC_SET_CMD, (int32_t)"init");
	run_core();
	run_procd();
	run_vfsd();

	//load procs before file system ready
	init_fs();

	switch_root();

	while(true) {
		proc_block(getpid(), (uint32_t)main);
	}
	return 0;
}
