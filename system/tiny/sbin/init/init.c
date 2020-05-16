#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/shm.h>
#include <sys/vfs.h>
#include <vprintf.h>
#include <sysinfo.h>
#include <sys/gpio.h>
#include <sys/ipc.h>
#include <sys/global.h>
#include <sys/proc.h>
#include <kevent.h>


static void b16_decode(const char *input, uint32_t input_len, char *output, uint32_t *output_len) {
  uint32_t i;
  for (i = 0; i < input_len / 2; i++) {
    uint8_t low = input[2 * i] - 'A';
    uint8_t high = input[2 * i + 1] - 'A';
    output[i] = low | (high << 4);
  }
  *output_len = input_len / 2;
}

static char* decode_fs(const char* data[], int size) {
  if(size <= 0)
    return NULL;

  char* elf = (char*)malloc(size);
  if(elf == NULL) {
    return NULL;
  }

  uint32_t i = 0;
  char* p = elf;
  while(1) {
    const char* s = data[i];
    if(s == NULL)
      break;
    uint32_t sz = 0;
    b16_decode(s, strlen(s), p , &sz);
    p += sz;
    i++;
  }
  return elf;
}

extern const char* vfsd_data[];
extern const int vfsd_size;
/*
extern const char* rootfsd_data[];
extern const int rootfsd_size;
*/

const char* rootfsd_data[] = { NULL };
const int rootfsd_size = 0;


static char* load_none_fs(const char* cmd, int32_t* sz) {
	if(strcmp(cmd, "/sbin/vfsd") == 0) {
		*sz = vfsd_size;
		return decode_fs(vfsd_data, vfsd_size);
	}
	else if(strcmp(cmd, "/sbin/rootfsd") == 0) {
		*sz = rootfsd_size;
		return decode_fs(rootfsd_data, rootfsd_size);
	}
	return NULL;
}

static int run_none_fs(const char* cmd) {
	kprintf(false, "init: %s ", cmd);
	int pid = fork();
	if(pid == 0) {
		str_t* fname = str_new("");
		str_to(cmd, ' ', fname, 1);
		int32_t sz = 0;
		void* data = load_none_fs(CS(fname), &sz);
		str_free(fname);

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

static int run(const char* cmd, bool prompt, bool wait) {
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
	else if(wait)
		proc_wait_ready(pid);

	if(prompt)
		kprintf(false, "[ok]\n");
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
		run(ln, true, true);
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
		run(ln, false, false);
	}
	close(fd);
}

void core(void);
static void run_core(void) {
	int pid = fork();
	if(pid == 0) {
		core();
	}
	else
		proc_wait_ready(pid);
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	kprintf(false, "\n[init process started]\n");
	run_core();

	run_none_fs("/sbin/vfsd");
	//mount root fs
	run_none_fs("/sbin/dev/rootfsd");
	//fs got ready.
	run("/sbin/procd", true, true);

	load_devs();

	setenv("OS", "mkos");
	setenv("PATH", "/sbin:/bin");

	run_procs();

	while(true) {
		sleep(1);
	}
	return 0;
}
