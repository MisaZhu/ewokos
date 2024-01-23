#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ewoksys/proc.h>
#include <ewoksys/ipc.h>
#include <ewoksys/core.h>

char* getcwd(char* buf, size_t size) {
	proto_t out;
	PF->init(&out);
	if(ipc_call(get_cored_pid(), CORE_CMD_GET_CWD, NULL, &out) == 0) {
		if(proto_read_int(&out) == 0) {
			strncpy(buf, proto_read_str(&out), size-1);
		}
	}
	PF->clear(&out);
	return buf;
}

