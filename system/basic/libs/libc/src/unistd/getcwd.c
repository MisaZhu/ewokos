#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/core.h>

char* getcwd(char* buf, uint32_t size) {
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

