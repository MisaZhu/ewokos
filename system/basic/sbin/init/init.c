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
#include <dirent.h>

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

static void load_devs(const char* cfg) {
	int fd = open(cfg, O_RDONLY);
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

static void load_init_devs(void) {
	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (int32_t)&sysinfo);
	char fn[FS_FULL_NAME_MAX];
	snprintf(fn, FS_FULL_NAME_MAX-1, "/etc/dev/arch/%s/init.dev", sysinfo.machine);
	load_devs(fn);
}

static void load_extra_devs(void) {
	const char* dirn = "/etc/dev/extra";
  DIR* dirp = opendir(dirn);
  if(dirp == NULL)
    return;
  while(1) {
    struct dirent* it = readdir(dirp);
    if(it == NULL)
      break;
		char fn[FS_FULL_NAME_MAX];
    snprintf(fn, FS_FULL_NAME_MAX-1, "%s/%s", dirn, it->d_name);
		load_devs(fn);
  }
  closedir(dirp);
}

static void load_sys_devs(void) {
	load_devs("/etc/dev/sys.dev");
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
		run(ln, false, false);
	}
	close(fd);
}

void core(void);
static void run_core(void) {
	klog("run init-core    ");
	int pid = fork();
	if(pid == 0) {
		syscall1(SYS_PROC_SET_CMD, (int32_t)"init-core");
		core();
	}
	else
		proc_wait_ready(pid);
	klog("[ok]\n");
}

int vfsd_main(void);
static void run_vfsd(void) {
	klog("run init-vfsd    ");
	int pid = fork();
	if(pid == 0) {
		syscall1(SYS_PROC_SET_CMD, (int32_t)"init-vfsd");
		vfsd_main();
	}
	else
		proc_wait_ready(pid);
	klog("[ok]\n");
}

int procd_main(void);
static void run_procd(void) {
	klog("run init-procd    ");
	int pid = fork();
	if(pid == 0) {
		syscall1(SYS_PROC_SET_CMD, (int32_t)"init-procd");
		procd_main();
	}
	else
		proc_wait_ready(pid);
	klog("[ok]\n");
}

void romfsd_main(void);
void sdfsd_main(void);
static void init_rootfs(void) {
	klog("run init-rootfsd    ");
	int pid = fork();
	if(pid == 0) {
		int32_t sz = syscall3(SYS_KROMFS_GET, 0, (int32_t)NULL, (int32_t)NULL);	
		if(sz < 0) {
			syscall1(SYS_PROC_SET_CMD, (int32_t)"init-sdfsd");
			sdfsd_main();
		}
		else {
			syscall1(SYS_PROC_SET_CMD, (int32_t)"init-romfsd");
			romfsd_main();
		}
	}
	else
		proc_wait_ready(pid);
	klog("[ok]\n");
}

static void init_tty_stdio(void) {
	int fd = open("/dev/tty0", 0);
	int fd_log = open("/dev/klog", 0);

	dup2(fd, 0);
	dup2(fd, 1);
	if(fd_log > 0)
		dup2(fd_log, 2);
	else
		dup2(fd, 2);

	run("/drivers/stdiod /dev/stdin", true, true);
	run("/drivers/stdiod /dev/stdout", true, true);
	run("/drivers/stdiod /dev/stderr", true, true);
}

static void switch_root(void) {
	int pid = fork();
	if(pid == 0) {
		setuid(0);
		load_init_devs();
		load_extra_devs();
		load_sys_devs();
		init_tty_stdio();
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
	init_rootfs();

	switch_root();

	while(true) {
		proc_block(getpid(), (uint32_t)main);
	}
	return 0;
}
