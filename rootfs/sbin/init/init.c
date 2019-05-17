#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <vfs/fs.h>
#include <sdread.h>
#include <kserv.h>
#include <syscall.h>
#include <tstr.h>
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

/*multi consoles*/
const char* cons[2] = {
	"/dev/console0",
	"/dev/console1"
};

static int32_t _num = 2;
static int32_t _si = 0;

static void cons_sw(void) {
	int32_t i = 0;
	_si++;
	if(_si >= _num)
		_si = 0;

	while(i < _num) {
		fs_fctrl(cons[i], 5, NULL, 0, NULL, 0); //disable
		i++;
	}
	fs_fctrl(cons[_si], 4, NULL, 0, NULL, 0); //disable
}

/*system global event*/
static void kevent_loop(void) {
	_num = 2;
	_si = 0;
	int fd = open("/dev/kevent0", O_RDONLY);
	if(fd < 0)
		return;

	while(true) {
		int32_t ev;
		int sz = read(fd, &ev, 4);
		if(sz < 0)
			break;
		if(sz == 0)
			continue;
		
		if(ev == KEV_TERMINATE) {//terminate
		}
		else if(ev == KEV_CONSOLE_SWITCH) {//switch
			cons_sw();
		}
	}
	close(fd);
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
	int pid = fork();
	if(pid == 0) {
		setenv("STDIO_DEV", "/dev/console0");
		setenv("CONSOLE", "1/2");
		exec("/sbin/session");
		return 0;
	}

	pid = fork();
	if(pid == 0) {
		setenv("STDIO_DEV", "/dev/console1");
		setenv("CONSOLE", "2/2");
		exec("/sbin/session");
		return 0;
	}

	pid = fork();
	if(pid == 0) {
		setenv("STDIO_DEV", "/dev/tty0");
		setenv("CONSOLE", "tty0");
		exec("/sbin/session");
		return 0;
	}

	kevent_loop();
	return 0;
}
