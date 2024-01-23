#include <unistd.h>
#include <stdlib.h>
#include <ewoksys/mstr.h>
#include <ewoksys/syscall.h>
#include <ewoksys/proc.h>
#include <ewoksys/vfs.h>

int exec(const char* cmd_line) {

	execve(cmd_line, "", "");
}

