#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <vfs/fs.h>
#include <kserv.h>
#include <syscall.h>

static int start_vfsd() {
	printf("start vfs service ... ");
	int pid = fork();
	if(pid == 0) { 
		exec("/sbin/vfsd");
	}
	kserv_wait("kserv.vfsd");
	printf("ok.\n");
	return 0;
}

static int mount_root() {
	printf("mount root fs from sdcard ...\n");
	int pid = fork();
	if(pid == 0) { 
		exec("/sbin/dev/sdcard");
	}
	kserv_wait("dev.sdcard");
	printf("root mounted.\n");
	return 0;
}

static int read_line(int fd, char* line, int sz) {
	int i = 0;
	while(i<sz) {
		char c;
		if(read(fd, &c, 1) <= 0) {
			line[i] = 0;
			return -1;
		}
		if(c == '\n') {
			line[i] = 0;
			break;
		}
		line[i++] = c;
	}
	return i;
}

static int run_init_procs(const char* fname) {
	int fd = open(fname, 0);
	if(fd < 0)
		return -1;

	char cmd[CMD_MAX];
	int i = 0;
	while(true) {
		i = read_line(fd, cmd, CMD_MAX-1);
		if(i < 0)
			break;
		if(i == 0)
			continue;

		int pid = fork();
		if(pid == 0) { 
			exec(cmd);
		}
	}
	close(fd);
	return 0;
}

static void session_loop() {
	while(1) {
		int pid = fork();
		if(pid == 0) {
			exec("/sbin/session");
		}
		else {
			wait(pid);
		}
	}
}

int main() {
	if(getpid() > 0) {
		printf("Panic: 'init' process can only run at boot time!\n");
		return -1;
	}
	start_vfsd();
	mount_root();
	run_init_procs("/etc/init/init.dev");
	run_init_procs("/etc/init/init.rd");

	/*set uid to root*/
	syscall2(SYSCALL_SET_UID, getpid(), 0);
	/*run 2 session for uart0 and framebuffer based console0*/
	int pid = fork();
	if(pid == 0) {
		setenv("STDIO_DEV", "/dev/console0");
		init_stdio();
		exec("/sbin/session");
		return 0;
	}
	else {
		setenv("STDIO_DEV", "/dev/tty0");
		session_loop();
	}
	return 0;
}
