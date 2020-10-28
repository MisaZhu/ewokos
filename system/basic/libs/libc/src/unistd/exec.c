#include <unistd.h>
#include <stdlib.h>
#include <sys/mstr.h>
#include <sys/syscall.h>
#include <sys/proc.h>
#include <sys/vfs.h>

int exec(const char* cmd_line) {
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

