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
	kserv_wait_by_pid(pid);
	printf("ok.\n");
	return 0;
}

char* from_sd(const char* fname, int32_t* sz);
static int run_init_dev(const char* fname) {
	int32_t size;
	char* data = from_sd(fname, &size);
	if(data == NULL)
		return -1;

	char* cmd = data;
	int i = 0;
	while(i < size) {
		if(data[i] == '\n' || data[i] == 0) {
			data[i] = 0;
			if(cmd[0] != 0 && cmd[0] != '#') {
				int pid = fork();
				if(pid == 0) { 
					exec(cmd);
				}
				kserv_wait_by_pid(pid);
			}
			cmd = data+i+1;
		}
		++i;
	}

	free(data);
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

static int run_init_servs(const char* fname) {
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
		if(cmd[0] != 0 && cmd[0] != '#') {
			int pid = fork();
			if(pid == 0) { 
				exec(cmd);
			}
			kserv_wait_by_pid(pid);
			printf("loaded serv: %s\n", cmd);
		}
	}
	close(fd);
	return 0;
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

		if(cmd[0] != 0 && cmd[0] != '#') {
			int pid = fork();
			if(pid == 0) { 
				exec(cmd);
			}
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

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	if(getpid() > 0) {
		printf("Panic: 'init' process can only run at boot time!\n");
		return -1;
	}
	start_vfsd();
	//mount_root();
	run_init_dev("/etc/init/init.dev");
	run_init_servs("/etc/init/init.serv");
	run_init_procs("/etc/init/init.rd");

	/*set uid to root*/
	syscall2(SYSCALL_SET_UID, getpid(), 0);
	/*run 2 session for uart0 and framebuffer based console0*/
	int pid = fork();
	if(pid == 0) {
		setenv("STDOUT_DEV", "/dev/console0");
		setenv("STDIN_DEV", "/dev/console0");
		init_stdio();
		exec("/sbin/session");
		return 0;
	}
	else {
		setenv("STDOUT_DEV", "/dev/tty0");
		setenv("STDIN_DEV", "/dev/tty0");
		session_loop();
	}
	return 0;
}
