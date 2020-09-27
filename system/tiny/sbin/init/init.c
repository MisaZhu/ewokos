#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/vfs.h>
#include <vprintf.h>
#include <sysinfo.h>
#include <sys/kprintf.h>
#include <sys/ipc.h>
#include <sys/proc.h>

static char* run_none_fs_kfs(const char* cmd, int32_t *size) {
	int32_t index = 0;
	char* data = NULL;
	while(true) {
		char name[FS_FULL_NAME_MAX];
		int32_t sz = syscall3(SYS_KFS_GET, index, (int32_t)name, (int32_t)NULL);	
		if(sz < 0) 
			break;
		if(sz > 0 && name[0] != 0 && strcmp(cmd, name) == 0)  {
			data = (char*)malloc(sz);
			sz = syscall3(SYS_KFS_GET, index, (int32_t)name, (int32_t)data);	
			*size = sz;
			return data;
		}
		index++;
	}
	return NULL;	
}

static int run_none_fs(const char* cmd) {
	kprintf(false, "init: %s ", cmd);
	int pid = fork();
	if(pid == 0) {
		char* data = NULL;
		int32_t sz = 0;
		data = run_none_fs_kfs(cmd, &sz);
		if(data == NULL) {
			kprintf(false, "[error!] (%s)\n", cmd);
			exit(-1);
		}
		proc_exec_elf(cmd, data, sz);
		free(data);
	}
	proc_wait_ready(pid);
	kprintf(false, "[ok]\n");
	return pid;
}

static int run(const char* cmd, bool prompt, bool wait) {
	if(prompt)
		kprintf(false, "init: %s ", cmd);

	int pid = fork();
	if(pid == 0) {
		setuid(0);
		if(exec(cmd) != 0) {
			if(prompt)
				kprintf(false, "[error!]\n");
			exit(-1);
		}
	}
	else if(wait)
		proc_wait_ready(pid);

	if(prompt)
		kprintf(false, "[ok]\n");
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
		syscall1(SYS_PROC_SET_CMD, (int32_t)"/sbin/init-core");
		core();
	}
	else
		proc_wait_ready(pid);
}

static void init_tty_stdio(void) {
	int fd = open("/dev/tty0", 0);
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	if(getuid() >= 0) {
		kprintf(false, "process 'init' can only loaded by kernel!\n");
		return -1;
	}

	kprintf(false, "\n[init process started]\n");
	run_core();

	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (int32_t)&sysinfo);

	run_none_fs("/sbin/procd");
	run_none_fs("/sbin/vfsd");
	run_none_fs("/drivers/rootfsd");

	load_devs();

	setenv("OS", "mkos");

	init_tty_stdio();
	run_procs();

	while(true) {
		proc_block(getpid(), 0);
		sleep(1);
	}
	return 0;
}
