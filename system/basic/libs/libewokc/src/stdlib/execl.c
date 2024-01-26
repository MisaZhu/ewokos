#include <unistd.h>
#include <stdlib.h>
#include <ewoksys/mstr.h>
#include <ewoksys/syscall.h>
#include <ewoksys/proc.h>
#include <ewoksys/vfs.h>

int execl(const char* cmd_line, const char* arg, ...) {
	str_t* cmd = str_new("");
	const char *p = cmd_line;
	while(*p != 0 && *p != ' ') {
		str_addc(cmd, *p);
		p++;
	}
	str_addc(cmd, 0);
	int sz;
	void* buf = vfs_readfile(cmd->cstr, &sz);
	str_free(cmd);
	if(buf == NULL) {
		return -1;
	}
	proc_exec_elf(cmd_line, buf, sz);
	free(buf);
	return 0;
}

