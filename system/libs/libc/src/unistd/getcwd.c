#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/proc.h>

char* getcwd(char* buf, uint32_t size) {
	proto_t out;
	PF->init(&out, NULL, 0);
	if(ipc_call(get_procd_pid(), PROC_CMD_GET_CWD, NULL, &out) == 0) {
		if(proto_read_int(&out) == 0) {
			strncpy(buf, proto_read_str(&out), size-1);
		}
	}
	PF->clear(&out);
	return buf;
}

