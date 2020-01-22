#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dev/fbinfo.h>
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
#include <rawdata.h>
#include <sys/global.h>
#include <graph/graph.h>
#include <console.h>
#include "sdinit.h"

static inline void wait_ready(int pid) {
	while(1) {
		if(dev_ping(pid) == 0)
			break;
		sleep(1);
	}
}

/*
static void run_init_sd(const char* cmd) {
	sysinfo_t sysinfo;
	syscall1(SYS_GET_SYSINFO, (int32_t)&sysinfo);
	char devfn[FS_FULL_NAME_MAX];
	snprintf(devfn, FS_FULL_NAME_MAX-1, "/sbin/dev/%s/%s", sysinfo.machine, cmd);

	kprintf("init: load sd %8s ", "");
	int pid = fork();
	if(pid == 0) {
		sdinit_init();
		ext2_t ext2;
		ext2_init(&ext2, sdinit_read, NULL);
		int32_t sz;
		void* data = ext2_readfile(&ext2, devfn, &sz);
		ext2_quit(&ext2);

		if(data == NULL) {
			kprintf("[error!] (%s)\n", devfn);
			exit(-1);
		}
		exec_elf(devfn, data, sz);
		free(data);
	}
	wait_ready(pid);
	kprintf("[ok]\n");
}
*/

static void run_init_root(const char* cmd) {
	kprintf("init: %16s ", "/");
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
			kprintf("[error!] (%s)\n", cmd);
			exit(-1);
		}
		exec_elf(cmd, data, sz);
		free(data);
	}
	wait_ready(pid);
	kprintf("[ok]\n");
}

static int run_dev(const char* cmd, bool prompt) {
	if(prompt)
		kprintf("init: %s ", cmd);

	int pid = fork();
	if(pid == 0) {
		if(exec(cmd) != 0) {
			if(prompt)
				kprintf("[error!]\n");
			exit(-1);
		}
	}
	else
		wait_ready(pid);

	if(prompt)
		kprintf("[ok]\n");
	return 0;
}

static void run(const char* cmd) {
	int pid = fork();
	if(pid == 0) {
		if(exec(cmd) != 0)
			kprintf("init: run %s [error!]", cmd);
	}
}

static void init_stdio(void) {
	int fd = open("/dev/tty0", 0);
	dup2(fd, 0);
	dup2(fd, 1);
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
		run_dev(ln, true);
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

	kprintf("\n[init process started]\n");
	//mount root fs
	//run_init_sd("sdd");
	run_init_root("/sbin/dev/rootfsd");

	load_devs();
	init_stdio();
	run_procs();

	while(1) {
		sleep(1);
	}
	return 0;
}
