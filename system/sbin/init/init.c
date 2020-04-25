#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <dev/device.h>
#include <sys/shm.h>
#include <sys/vfs.h>
#include <ext2fs.h>
#include <vprintf.h>
#include <sysinfo.h>
#include <sconf.h>
#include <sys/sd.h>
#include <sys/gpio.h>
#include <sys/ipc.h>
#include <rawdata.h>
#include <sys/global.h>
#include <sys/proc.h>
#include <graph/graph.h>
#include <kevent.h>
#include "sdinit.h"

static int run_none_fs(const char* cmd) {
	kprintf(false, "init: %s ", cmd);
	int pid = fork();
	if(pid == 0) {
		sd_init();
		ext2_t ext2;
		ext2_init(&ext2, sd_read, sd_write);
		str_t* fname = str_new("");
		str_to(cmd, ' ', fname, 1);
		int32_t sz;
		void* data = ext2_readfile(&ext2, fname->cstr, &sz);
		str_free(fname);
		ext2_quit(&ext2);

		if(data == NULL) {
			kprintf(false, "[error!] (%s)\n", cmd);
			exit(-1);
		}
		exec_elf(cmd, data, sz);
		free(data);
	}
	proc_wait_ready(pid);
	kprintf(false, "[ok]\n");
	return pid;
}

static int run_serv(const char* cmd, bool prompt) {
	if(prompt)
		kprintf(false, "init: %s ", cmd);

	int pid = fork();
	if(pid == 0) {
		if(exec(cmd) != 0) {
			if(prompt)
				kprintf(false, "[error!]\n");
			exit(-1);
		}
	}
	else
		proc_wait_ready(pid);

	if(prompt)
		kprintf(false, "[ok]\n");
	return 0;
}

static void run(const char* cmd) {
	int pid = fork();
	if(pid == 0) {
		if(exec(cmd) != 0)
			kprintf(false, "init: run %s [error!]", cmd);
	}
}

static void init_stdio(void) {
	int fd = open("/dev/tty0", 0);
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);
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
	sysinfo_t sysinfo;
	syscall1(SYS_GET_SYSINFO, (int32_t)&sysinfo);
	char fn[FS_FULL_NAME_MAX];
	snprintf(fn, FS_FULL_NAME_MAX-1, "/etc/arch/%s.dev", sysinfo.machine);
	int fd = open(fn, O_RDONLY);
	if(fd < 0)
		return;

	while(true) {
		const char* ln = read_line(fd);
		if(ln == NULL)
			break;
		if(ln[0] == 0 || ln[0] == '#')
			continue;
		run_serv(ln, true);
	}
	close(fd);
}

static void run_procs(void) {
	int fd = open("/etc/init.rd", O_RDONLY);
	if(fd < 0)
		return;

	while(true) {
		const char* ln = read_line(fd);
		if(ln == NULL)
			break;
		if(ln[0] == 0 || ln[0] == '#')
			continue;
		run(ln);
	}
	close(fd);
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	setenv("OS", "mkos");
	setenv("PATH", "/sbin:/bin");

	kprintf(false, "\n[init process started]\n");
	run_none_fs("/sbin/kservd"); //have to be pid 1
	run_none_fs("/sbin/vfsd");
	//mount root fs
	run_none_fs("/sbin/dev/rootfsd");

	//fs got ready.
	run_serv("/sbin/keventd", true);

	load_devs();
	init_stdio();
	run_procs();

	while(1) {
		sleep(1);
	}
	return 0;
}
