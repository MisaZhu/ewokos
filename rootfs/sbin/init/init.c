#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <vfs/fs.h>
#include <sdread.h>
#include <kserv.h>
#include <syscall.h>
#include <tstr.h>
#include <kstring.h>
#include <kevent.h>

static int start_vfsd(void) {
	printf("start vfs service ... ");
	int pid = fork();
	if(pid == 0) { 
		exec("/sbin/vfsd");
	}
	kserv_wait_by_pid(pid);
	printf("ok.\n");
	return 0;
}

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
	setenv("DEV_INITED", "yes");
	return 0;
}

static int read_line(int fd, tstr_t* line) {
	int i = 0;
	while(true) {
		char c;
		if(read(fd, &c, 1) <= 0) {
			tstr_addc(line, 0);
			return -1;
		}
		if(c == '\n') {
			tstr_addc(line, 0);
			break;
		}
		tstr_addc(line, c);
		i++;
	}
	return i;
}

static int run_init_procs(const char* fname, bool wait) {
	int fd = open(fname, 0);
	if(fd < 0)
		return -1;

	tstr_t* line = tstr_new("", MFS);
	int i = 0;
	while(true) {
		i = read_line(fd, line);
		if(i < 0)
			break;
		if(i == 0)
			continue;
		const char* cmd = CS(line);
		if(cmd[0] != 0 && cmd[0] != '#') {
			int pid = fork();
			if(pid == 0) { 
				exec(cmd);
			}
			if(wait)
				kserv_wait_by_pid(pid);
		}
		tstr_empty(line);
	}
	tstr_free(line);
	close(fd);
	return 0;
}

static void kevent_loop(void) { //system global event
	int fd = open("/dev/kevent0", O_RDONLY);
	if(fd < 0)
		return;

	while(true) {
		int32_t ev;
		int sz = read(fd, &ev, 4);
		if(sz < 0)
			break;
		if(sz > 0) {
			if(ev == KEV_TERMINATE) {//TODO terminate
			}
		}
		usleep(100000);
	}
	close(fd);
}

static void run_consoles(void) {
	int pid = fork();
	if(pid == 0) {
		setenv("STDIO_DEV", "/dev/tty0");
		setenv("CONSOLE", "tty0");
		exec("/sbin/session");
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
	run_init_dev("/etc/init/init.dev");
	run_init_procs("/etc/init/init.serv", true);
	run_init_procs("/etc/init/init.rd", false);
	/*set uid to root*/
	syscall2(SYSCALL_SET_UID, getpid(), 0);
	/*run 2 session for uart0 and framebuffer based console0*/
	run_consoles();
	kevent_loop();
	return 0;
}
