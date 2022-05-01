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
#include <sd/sd.h>
#include <ext2/ext2fs.h>

static int32_t fd_console = -1;
static void outc(char c, void* p) {
	str_t *buf = (str_t *)p;
	str_addc(buf, c);
}

static void out(const char *format, ...) {
	str_t *str = str_new(NULL);
	va_list ap;
	va_start(ap, format);
	v_printf(outc, str, format, ap);
	va_end(ap);
	klog("%s", str->cstr);
	if(fd_console > 0)
		dprintf(2, "%s", str->cstr);
	str_free(str);
}

static void* sd_read_ext2(const char* fname, int32_t* size) {
	ext2_t ext2;
	ext2_init(&ext2, sd_read, NULL);
	void* ret = ext2_readfile(&ext2, fname, size);
	ext2_quit(&ext2);
	return ret;
}

static int32_t exec_from_sd(const char* prog) {
	int32_t sz;

	if(sd_init() != 0) {
		return -1;
	}

	char* elf = sd_read_ext2(prog, &sz);
	if(elf != NULL) {
		int32_t res = syscall3(SYS_EXEC_ELF, (int32_t)prog, (int32_t)elf, sz);
		free(elf);
		if(res == 0) {
			return res;
		}
	}
	return -1;
}

static int check_file(const char* cmd_line) {
  str_t* cmd = str_new("");
  const char *p = cmd_line;
  while(*p != 0 && *p != ' ') {
    str_addc(cmd, *p);
    p++;
  }
  str_addc(cmd, 0);
	int res = vfs_access(cmd->cstr);
  str_free(cmd);
	return res;
}

static int run(const char* cmd, bool prompt, bool wait) {
	if(check_file(cmd) != 0) {
		out("init: file not found! [%s]\n", cmd);
		return -1;
	}

	if(prompt)
		out("init: %s ", cmd);

	int pid = fork();
	if(pid == 0) {
		proc_detach();

		if(exec(cmd) != 0) {
			if(prompt)
				out("[error!]\n");
			exit(-1);
		}
	}
	else if(wait)
		proc_wait_ready(pid);

	if(prompt)
		out("[ok]\n");
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

static int load_devs(const char* cfg) {
	int fd = open(cfg, O_RDONLY);
	if(fd < 0)
		return -1;

	while(true) {
		const char* ln = read_line(fd);
		if(ln == NULL)
			break;
		if(ln[0] == 0 || ln[0] == '#')
			continue;
		run(ln, true, true);
	}
	close(fd);
	return 0;
}

static void load_arch_devs(void) {
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

static void load_console(void) {
	if(load_devs("/etc/dev/console.dev") != 0)
		return;

	fd_console = open("/dev/console0", 0);
	if(fd_console < 0)
		return;

	const char* s = ""
			"+-----Ewok micro-kernel OS-----------------------+\n"
			"| https://github.com/MisaZhu/EwokOS.git          |\n"
			"+------------------------------------------------+\n";
	dprintf(fd_console, "%s", s);
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

static void run_core(void) {
	const char* fname = "/sbin/core";
	out("init: %s    ", fname);
	int pid = fork();
	if(pid == 0) {
		exec_from_sd(fname);
	}
	else
		proc_wait_ready(pid);
	out("[ok]\n");
}

static void run_vfsd(void) {
	const char* fname = "/sbin/vfsd";
	out("init: %s    ", fname);

	int pid = fork();
	if(pid == 0) {
		exec_from_sd(fname);
	}
	else
		proc_wait_ready(pid);
	out("[ok]\n");
}

static void run_rootfsd(void) {
	const char* fname = "/sbin/sdfsd";
	out("init: %s    ", fname);

	int pid = fork();
	if(pid == 0) {
		exec_from_sd(fname);
	}
	else
		proc_wait_ready(pid);
}

static void init_tty_stdio(void) {
	int fd = open("/dev/tty0", 0);
	if(fd > 0) {
		dup2(fd, 0);
		dup2(fd, 1);
		dup2(fd, 2);
		close(fd);
	}

	if(fd_console > 0) {
		dup2(fd_console, 2);
		close(fd_console);
	}
}

static void switch_root(void) {
	int pid = fork();
	if(pid == 0) {
		setuid(0);
		load_arch_devs();
		load_console();
		init_tty_stdio();
		load_extra_devs();
		load_sys_devs();
		run_procs();
		exit(0);
	}
}

static void halt(void) {
	uint32_t i = 0;
	while(1) {
		while(i++ < 0x000fffff);
		sleep(0);
		i = 0;	
	}
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	fd_console = -1;

	if(getuid() >= 0) {
		out("process 'init' can only loaded by kernel!\n");
		return -1;
	}

	if(getpid() != 0) //not first proc, must be cpu core idle
		halt();
	else
		syscall1(SYS_PROC_SET_CMD, (int32_t)"/sbin/init");

	out("\n[init process started]\n");
	run_core();
	run_vfsd();
	run_rootfsd();

	switch_root();
	while(true) {
		proc_block(getpid(), (uint32_t)main);
	}
	return 0;
}
