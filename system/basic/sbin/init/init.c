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

static void run_before_vfs(const char* cmd) {
	out("init: %s    ", cmd);

	int pid = fork();
	if(pid == 0) {
		exec_from_sd(cmd);
	}
	else
		proc_wait_ready(pid);
	out("[ok]\n");
}

static void switch_root(void) {
	int pid = fork();
	if(pid == 0) {
		setuid(0);
		//load_arch_devs();
		out("\ninit: loading init.rd\n");
		exec("/bin/shell /etc/init.rd");
	}
}

static void halt(void) {
	while(1);
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	if(getuid() >= 0) {
		out("process 'init' can only loaded by kernel!\n");
		return -1;
	}

	if(getpid() != 0) //not first proc, must be cpu core idle
		halt();
	else
		syscall1(SYS_PROC_SET_CMD, (int32_t)"/sbin/init");

	out("\n[init process started]\n");
	run_before_vfs("/sbin/core");
	run_before_vfs("/sbin/vfsd");
	run_before_vfs("/sbin/sdfsd");

	switch_root();
	while(true) {
		proc_block(getpid(), (uint32_t)main);
	}
	return 0;
}
