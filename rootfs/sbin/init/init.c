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

static int32_t _console_num = 0;

static int run_init_dev(const char* fname) {
	int32_t size;
	char* data = from_sd(fname, &size);
	if(data == NULL)
		return -1;

	_console_num = 0;
	char* cmd = data;
	int i = 0;
	while(i < size) {
		if(data[i] == '\n' || data[i] == 0) {
			data[i] = 0;
			if(cmd[0] != 0 && cmd[0] != '#') {
				if(strstr(cmd, "consoled") != NULL)
					_console_num++;
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

static int32_t _console_index = 0;

static void cons_sw(void) {
	int32_t i = 0;
	_console_index++;
	if(_console_index > _console_num)
		_console_index = 0;

	char dev[SHORT_NAME_MAX];	
	strcpy(dev, "/dev/xman0");
	fs_fctrl(dev, FS_CTRL_DISABLE, NULL, NULL); //enable

	while(i < _console_num) {
		snprintf(dev, SHORT_NAME_MAX-1, "/dev/console%d", i);
		fs_fctrl(dev, FS_CTRL_DISABLE, NULL, NULL); //disable
		i++;
	}

	if(_console_index < _console_num) {
		snprintf(dev, SHORT_NAME_MAX-1, "/dev/console%d", _console_index);
		fs_fctrl(dev, FS_CTRL_ENABLE, NULL, NULL); //enable
	}
	else if(_console_index == _console_num) { //xman
		strcpy(dev, "/dev/xman0");
		fs_fctrl(dev, FS_CTRL_ENABLE, NULL, NULL); //enable
	}
}

/*system global event*/
static void kevent_loop(void) {
	_console_index = 0;
	int fd = open("/dev/kevent0", O_RDONLY);
	if(fd < 0)
		return;

	while(true) {
		int32_t ev;
		int sz = read(fd, &ev, 4);
		if(sz < 0)
			break;
		if(sz > 0) {
			if(ev == KEV_TERMINATE) {//terminate
			}
			else if(ev == KEV_CONSOLE_SWITCH) {//switch
				cons_sw();
			}
		}
		sleep(0);
	}
	close(fd);
}

static void run_consoles(void) {
	int pid = 0;
	int i;	
	for(i=0; i<_console_num; i++) {	
		char dev[SHORT_NAME_MAX];	
		char name[SHORT_NAME_MAX];	
		snprintf(dev, SHORT_NAME_MAX-1, "/dev/console%d", i);
		snprintf(name, SHORT_NAME_MAX-1, "%d/%d", i+1, _console_num);
		pid = fork();
		if(pid == 0) {
			setenv("STDIO_DEV", dev);
			setenv("CONSOLE", name);
			exec("/sbin/session");
			return;
		}
	}

	pid = fork();
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
